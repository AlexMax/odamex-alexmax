// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2011 by The Odamex Team.
// Copyright (C) 2012 by Alex Mayfield.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//   Serverside banlist handling.
//
//-----------------------------------------------------------------------------

#include <sstream>
#include <string>

//#include <curl/curl.h>

#include "c_dispatch.h"
#include "cmdlib.h"
#include "d_player.h"
#include "sv_banlist.h"
#include "sv_main.h"

EXTERN_CVAR(sv_email)

Banlist banlist;

//// IPRange ////

// Constructor
IPRange::IPRange() {
	for (byte i = 0;i < 4;i++) {
		this->ip[i] = (byte)0;
		this->mask[i] = false;
	}
}

// Check a given address against the ip + range in the object.
bool IPRange::check(const netadr_t& address) {
	for (byte i = 0;i < 4;i++) {
		if (!(this->ip[i] == address.ip[i] || this->mask[i] == true)) {
			return false;
		}
	}

	return true;
}

// Check a given string address against the ip + range in the object.
bool IPRange::check(const std::string& address) {
	StringTokens tokens = TokenizeString(address, ".");

	// An IP address contains 4 octets
	if (tokens.size() != 4) {
		return false;
	}

	for (byte i = 0;i < 4;i++) {
		// * means that octet is masked and we will accept any byte
		if (tokens[i].compare("*") == 0) {
			continue;
		}

		// Convert string into byte.
		unsigned short octet = 0;
		std::istringstream buffer(tokens[i]);
		buffer >> octet;
		if (!buffer) {
			return false;
		}

		if (!(this->ip[i] == octet || this->mask[i] == true)) {
			return false;
		}
	}

	return true;
}

// Set the object's range to a specific address.
void IPRange::set(const netadr_t& address) {
	for (byte i = 0;i < 4;i++) {
		this->ip[i] = address.ip[i];
		this->mask[i] = false;
	}
}

// Set the object's range against the given address in string form.
bool IPRange::set(const std::string& input) {
	StringTokens tokens = TokenizeString(input, ".");

	// An IP address contains 4 octets
	if (tokens.size() != 4) {
		return false;
	}

	for (byte i = 0;i < 4;i++) {
		// * means that octet is masked
		if (tokens[i].compare("*") == 0) {
			this->mask[i] = true;
			continue;
		}
		this->mask[i] = false;

		// Convert string into byte.
		unsigned short octet = 0;
		std::istringstream buffer(tokens[i]);
		buffer >> octet;
		if (!buffer) {
			return false;
		}

		this->ip[i] = (byte)octet;
	}

	return true;
}

// Return the range as a string, with stars representing masked octets.
std::string IPRange::string() {
	std::ostringstream buffer;

	for (byte i = 0;i < 4;i++) {
		if (mask[i]) {
			buffer << '*';
		} else {
			buffer << (unsigned short)this->ip[i];
		}

		if (i < 3) {
			buffer << '.';
		}
	}

	return buffer.str();
}

//// Banlist ////

bool Banlist::add(const std::string& address, const time_t expire,
                  const std::string& name, const std::string& reason) {
	Ban ban;

	// Did we pass a valid address?
	if (!ban.range.set(address)) {
		return false;
	}

	// Fill in the rest of the ban information
	ban.expire = expire;
	ban.name = name;
	ban.reason = reason;

	// Add the ban to the banlist
	this->banlist.push_back(ban);

	return true;
}

// We have a specific client that we want to add to the banlist.
bool Banlist::add(player_t& player, const time_t expire,
                  const std::string& reason) {
	// Player must be valid.
	if (!validplayer(player)) {
		return false;
	}

	// Fill in ban info
	Ban ban;
	ban.expire = expire;
	ban.name = player.userinfo.netname;
	ban.range.set(player.client.address);
	ban.reason = reason;

	// Add the ban to the banlist
	this->banlist.push_back(ban);

	return true;
}

// Check a given address against the banlist.  Sets baninfo and returns
// true if passed address is banned, otherwise returns false.
bool Banlist::check(const netadr_t& address, Ban& baninfo) {
	for (std::vector<Ban>::iterator it = this->banlist.begin();
		 it != this->banlist.end();++it) {
		if (it->range.check(address) && (it->expire == 0 ||
										 it->expire > time(NULL))) {
			baninfo = *it;
			return true;
		}
	}

	return false;
}

// Return a complete list of bans.
bool Banlist::query(banlist_results_t &result) {
	// No banlist?  Return an error state.
	if (this->banlist.empty()) {
		return false;
	}

	result.reserve(this->banlist.size());
	for (size_t i = 0;i < banlist.size();i++) {
		result.push_back(banlist_result_t(i, &(this->banlist[i])));
	}

	return true;
}

// Run a query on the banlist and return a list of all matching bans.  Matches
// against IP address/range and partial name.
bool Banlist::query(const std::string &query, banlist_results_t &result) {
	// No banlist?  Return an error state.
	if (this->banlist.empty()) {
		return false;
	}

	// No query?  Return everything.
	if (query.empty()) {
		return this->query(result);
	}

	std::string pattern = "*" + (query) + "*";
	for (size_t i = 0;i < this->banlist.size();i++) {
		bool f_ip = this->banlist[i].range.check(query);
		bool f_name = CheckWildcards(pattern.c_str(),
									 this->banlist[i].name.c_str());
		if (f_ip || f_name) {
			result.push_back(banlist_result_t(i, &(this->banlist[i])));
		}
	}
	return true;
}

// Remove a ban from the banlist by index.
bool Banlist::remove(size_t index) {
	// No banlist?  Return an error state.
	if (this->banlist.empty()) {
		return false;
	}

	// Invalid index?  Return an error state.
	if (this->banlist.size() <= index) {
		return false;
	}

	this->banlist.erase(this->banlist.begin() + index);
	return true;
}

//// Console commands ////

// Ban bans a player by player id.
BEGIN_COMMAND (ban) {
	std::vector<std::string> arguments = VectorArgs(argc, argv);

	// We need at least one argument.
	if (arguments.size() < 1) {
		Printf(PRINT_HIGH, "Usage: ban <player id> [ban length] [reason].\n");
		return;
	}

	size_t pid;
	std::istringstream buffer(arguments[0]);
	buffer >> pid;
	if (!buffer) {
		Printf(PRINT_HIGH, "ban: need a player id.\n");
		return;
	}

	player_t &player = idplayer(pid);
	if (!validplayer(player)) {
		Printf(PRINT_HIGH, "ban: %d is not a valid player id.\n", pid);
		return;
	}

	// If a length is specified, turn the length into an expire time.
	time_t tim;
	if (arguments.size() > 1) {
		if (!StrToTime(arguments[1], tim)) {
			Printf(PRINT_HIGH, "ban: invalid ban time (try a period of time like \"2 hours\" or \"permanent\")\n");
			return;
		}
	} else {
		// Default is a permaban.
		tim = 0;
	}

	// If a reason is specified, add it too.
	std::string reason;
	if (arguments.size() > 2) {
		// Account for people who forget their double-quotes.
		arguments.erase(arguments.begin(), arguments.begin() + 2);
		reason = JoinStrings(arguments, " ");
	}

	// Add the ban and kick the player.
	banlist.add(player, tim, reason);
	SV_KickPlayer(player, reason);
} END_COMMAND (ban)

// addban adds a ban by IP address.
BEGIN_COMMAND (addban) {
	std::vector<std::string> arguments = VectorArgs(argc, argv);

	// We need at least one argument.
	if (arguments.size() < 1) {
		Printf(PRINT_HIGH, "Usage: addban <ip address or ip range> [ban length] [player name] [reason]\n");
		return;
	}

	std::string address = arguments[0];

	// If a length is specified, turn the length into an expire time.
	time_t tim;
	if (arguments.size() > 1) {
		if (!StrToTime(arguments[1], tim)) {
			Printf(PRINT_HIGH, "addban: invalid ban time (try a period of time like \"2 hours\" or \"permanent\")\n");
			return;
		}
	} else {
		// Default is a permaban.
		tim = 0;
	}

	// If the player's name is specified, add it too.
	std::string name;
	if (arguments.size() > 2) {
		name = arguments[2];
	}

	// If a reason is specified, add it too.
	std::string reason;
	if (arguments.size() > 3) {
		// Account for people who forget their double-quotes.
		arguments.erase(arguments.begin(), arguments.begin() + 3);
		reason = JoinStrings(arguments, " ");
	}

	banlist.add(address, tim, name, reason);
} END_COMMAND (addban)

BEGIN_COMMAND (delban) {
	std::vector<std::string> arguments = VectorArgs(argc, argv);

	// We need at least one argument.
	if (arguments.size() < 1) {
		Printf(PRINT_HIGH, "Usage: delban <banlist index>\n");
		return;
	}

	size_t bid;
	std::istringstream buffer(arguments[0]);
	buffer >> bid;
	if (!buffer || bid == 0) {
		Printf(PRINT_HIGH, "delban: banlist index must be a nonzero number.\n");
		return;
	}

	if (!banlist.remove(bid - 1)) {
		Printf(PRINT_HIGH, "delban: banlist index does not exist.\n");
		return;
	}
} END_COMMAND (delban)

BEGIN_COMMAND (banlist) {
	std::vector<std::string> arguments = VectorArgs(argc, argv);

	banlist_results_t result;
	if (!banlist.query(JoinStrings(arguments, " "), result)) {
		Printf(PRINT_HIGH, "banlist: banlist is empty.\n");
		return;
	}

	if (result.empty()) {
		Printf(PRINT_HIGH, "banlist: no results found.\n");
		return;
	}

	char expire[20];
	tm *tmp;

	for (banlist_results_t::iterator it = result.begin();
		 it != result.end();++it) {
		std::ostringstream buffer;
		buffer << it->first + 1 << ". " << it->second->range.string();

		if (it->second->expire == 0) {
			strncpy(expire, "Permanent", 19);
		} else {
			tmp = localtime(&(it->second->expire));
			if (!strftime(expire, 20, "%Y-%m-%d %H:%M:%S", tmp)) {
				strncpy(expire, "???", 19);
			}
		}
		buffer << " " << expire;

		bool has_name = !it->second->name.empty();
		bool has_reason = !it->second->reason.empty();
		if (has_name || has_reason) {
			buffer << " (";
			if (!has_name) {
				buffer << "\"" << it->second->reason << "\"";
			} else if (!has_reason) {
				buffer << it->second->name;
			} else {
				buffer << it->second->name << ": \"" << it->second->reason << "\"";
			}
			buffer << ")";
		}

		Printf(PRINT_HIGH, "%s", buffer.str().c_str());
	}
} END_COMMAND (banlist)

// Check to see if a client is on the banlist, and kick them out of the server
// if they are.  Returns true if the player was banned.
bool SV_BanCheck(client_t *cl, int n) {
	Ban ban;
	if (!banlist.check(cl->address, ban)) {
		return false;
	}

	// TODO: Whitelisting

	std::ostringstream buffer;
	if (ban.expire == 0) {
		buffer << "You are indefinitely banned from this server.\n";
	} else {
		buffer << "You are banned from this server until ";

		char tbuffer[32];
		if (strftime(tbuffer, 32, "%c %Z", localtime(&ban.expire))) {
			buffer << tbuffer << ".\n";
		} else {
			buffer << ban.expire << " seconds after midnight on January 1st, 1970.\n";
		}
	}

	int name = ban.name.compare("");
	int reason = ban.reason.compare("");
	if (name != 0 || reason != 0) {
		buffer << "The given reason was: \"";
		if (name != 0 && reason == 0) {
			buffer << ban.name;
		} else if (name == 0 && reason != 0) {
			buffer << ban.reason;
		} else {
			buffer << ban.name << ": " << ban.reason;
		}
		buffer << "\"\n";
	}

	if (*(sv_email.cstring()) != 0) {
		buffer << "The server host can be contacted at ";
		buffer << sv_email.cstring();
		buffer << " if you feel this ban is in error or wish to contest it.";
	}

	// Log the banned connection attempt to the server.
	Printf(PRINT_HIGH, "%s is banned, dropping client.\n", NET_AdrToString(cl->address));

	// Send the message to the client.
	SV_ClientPrintf(cl, PRINT_HIGH, "%s", buffer.str().c_str());

	return true;
}

//// Old banlist code below ////

EXTERN_CVAR(sv_email)

// GhostlyDeath -- Someone might want to do IPv6 junk
#define IPADDRSIZE 4

// ban everyone! =)
const short RANGEBAN = -1;

typedef struct {
	short ip[IPADDRSIZE];
	std::string Reason;
} BanEntry_t;

// People who are banned
std::vector<BanEntry_t> BanList;

// People who are [accidently] banned but can get inside
std::vector<BanEntry_t> WhiteList;

//
// Nes - IP Lists: bans(BanList), exceptions(WhiteList)
//
void SV_BanStringify(std::string *ToStr = NULL, short *ip = NULL)
{
	if (!ToStr || !ip)
		return;

	for (int i = 0; i < IPADDRSIZE; i++)
	{
		if (ip[i] == RANGEBAN)
			*ToStr += '*';
		else
		{
			char bleh[5];
			sprintf(bleh, "%i", ip[i]);
			*ToStr += bleh;
		}

		if (i < (IPADDRSIZE - 1))
			*ToStr += '.';
	}
}

// Nes - Make IP for the temporary BanEntry.
void SV_IPListMakeIP (BanEntry_t *tBan, std::string IPtoBan)
{
	std::string Oct;//, Oct2, Oct3, Oct4;

	// There is garbage in IPtoBan
	IPtoBan = IPtoBan.substr(0, IPtoBan.find(' '));

	for (int i = 0; i < IPADDRSIZE; i++)
	{
		int loc = 0;
		char *seek;

		Oct = IPtoBan.substr(0, Oct.find("."));
		IPtoBan = IPtoBan.substr(IPtoBan.find(".") + 1);

		seek = const_cast<char*>(Oct.c_str());

		while (*seek)
		{
			if (*seek == '.')
				break;
			loc++;
			seek++;
		}

		Oct = Oct.substr(0, loc);

		if ((*(Oct.c_str()) == '*') || (*(Oct.c_str()) == 0))
			(*tBan).ip[i] = RANGEBAN;
		else
			(*tBan).ip[i] = atoi(Oct.c_str());
	}
}

// Nes - Add IP to a certain IP list.
void SV_IPListAdd (std::vector<BanEntry_t> *list, std::string listname, std::string IPtoBan, std::string reason)
{
	BanEntry_t tBan;	// GhostlyDeath -- Temporary Ban Holder
	std::string IP;

	SV_IPListMakeIP(&tBan, IPtoBan);

	for (size_t i = 0; i < (*list).size(); i++)
	{
		bool match = false;

		for (int j = 0; j < IPADDRSIZE; j++)
			if (((tBan.ip[j] == (*list)[i].ip[j]) || ((*list)[i].ip[j] == RANGEBAN)) &&
				(((j > 0 && match) || (j == 0 && !match))))
				match = true;
			else
			{
				match = false;
				break;
			}

		if (match)
		{
			SV_BanStringify(&IP, tBan.ip);
			Printf(PRINT_HIGH, "%s on %s already exists!\n", listname.c_str(), IP.c_str());
			return;
		}
	}

	tBan.Reason = reason;

	(*list).push_back(tBan);
	SV_BanStringify(&IP, tBan.ip);
	Printf(PRINT_HIGH, "%s on %s added.\n", listname.c_str(), IP.c_str());
}


// Nes - List a certain IP list.
void SV_IPListDisplay (std::vector<BanEntry_t> *list, std::string listname)
{
	if (!(*list).size())
		Printf(PRINT_HIGH, "%s list is empty.\n", listname.c_str());
	else {
		for (size_t i = 0; i < (*list).size(); i++)
		{
			std::string IP;
			SV_BanStringify(&IP, (*list)[i].ip);
			Printf(PRINT_HIGH, "%s #%i: %s (Reason: %s)\n", listname.c_str(), i + 1, IP.c_str(), (*list)[i].Reason.c_str());
		}

		Printf(PRINT_HIGH, "%s list has %i entries.\n", listname.c_str(), (*list).size());
	}
}

// Nes - Clears a certain IP list.
void SV_IPListClear (std::vector<BanEntry_t> *list, std::string listname)
{
	if (!(*list).size())
		Printf(PRINT_HIGH, "%s list is already empty!\n", listname.c_str());
	else {
		Printf(PRINT_HIGH, "All %i %ss removed.\n", (*list).size(), listname.c_str());
		(*list).clear();
	}
}

// Nes - Delete IP from a certain IP list.
void SV_IPListDelete(std::vector<BanEntry_t> *list, std::string listname, std::string IPtoBan) {
	if (!(*list).size()) {
		Printf(PRINT_HIGH, "%s list is empty.\n", listname.c_str());
	} else {
		BanEntry_t tBan;	// GhostlyDeath -- Temporary Ban Holder
		std::string IP;
		int RemovalCount = 0;
		size_t i;

		SV_IPListMakeIP(&tBan, IPtoBan);

		for (i = 0;i < (*list).size();i++) {
			bool match = false;

			for (int j = 0;j < IPADDRSIZE; j++) {
				if (tBan.ip[j] == (*list)[i].ip[j]) {
					match = true;
				} else {
					match = false;
					break;
				}
			}

			if (match) {
				(*list)[i].ip[0] = 32000;
				RemovalCount++;
			}
		}

		i = 0;

		while (i < (*list).size()) {
			if ((*list)[i].ip[0] == 32000) {
				(*list).erase((*list).begin() + i);
			} else {
				i++;
			}
		}

		if (RemovalCount == 0) {
			Printf(PRINT_HIGH, "%s entry not found.\n", listname.c_str());
		} else {
			Printf(PRINT_HIGH, "%i %ss removed.\n", RemovalCount, listname.c_str());
		}
	}
}

//
//  SV_BanCheck
//
//  Checks a connecting player against a banlist
//
/*bool SV_BanCheck (client_t *cl, int n)
{
	for (size_t i = 0; i < BanList.size(); i++)
	{
		bool match = false;
		bool exception = false;

		for (int j = 0; j < IPADDRSIZE; j++)
		{
			if (((cl->address.ip[j] == BanList[i].ip[j]) || (BanList[i].ip[j] == RANGEBAN)) &&
				(((j > 0 && match) || (j == 0 && !match))))
				match = true;
			else
			{
				match = false;
				break;
			}
		}

		// Now see if there is an exception on our ban...
		if (WhiteList.empty() == false)
		{
			for (size_t k = 0; k < WhiteList.size(); k++)
			{
				exception = false;

				for (int j = 0; j < IPADDRSIZE; j++)
				{
					if (((cl->address.ip[j] == WhiteList[k].ip[j]) || (WhiteList[k].ip[j] == RANGEBAN)) &&
						(((j > 0 && exception) || (j == 0 && !exception))))
						exception = true;
					else
					{
						exception = false;
						break;
					}
				}

				if (exception)
					break;	// we already know they are allowed in
			}
		}

		if (match && !exception)
		{
			std::string BanStr;
			BanStr += "\nYou are banned! (reason: ";
			//if (*(BanList[i].Reason.c_str()))
				BanStr += BanList[i].Reason;
			//else
			//	BanStr += "none given";
			BanStr += ")\n";

			MSG_WriteMarker   (&cl->reliablebuf, svc_print);
			MSG_WriteByte   (&cl->reliablebuf, PRINT_HIGH);
			MSG_WriteString (&cl->reliablebuf, BanStr.c_str());

			// GhostlyDeath -- Do we include the e-mail or no?
			if (*(sv_email.cstring()) == 0)
			{
				MSG_WriteMarker(&cl->reliablebuf, svc_print);
				MSG_WriteByte(&cl->reliablebuf, PRINT_HIGH);
				MSG_WriteString(&cl->reliablebuf, "If you feel there has been an error, contact the server host. (No e-mail given)\n");
			}
			else
			{
				std::string ErrorStr;
				ErrorStr += "If you feel there has been an error, contact the server host at ";
				ErrorStr += sv_email.cstring();
				ErrorStr += "\n\n";
				MSG_WriteMarker(&cl->reliablebuf, svc_print);
				MSG_WriteByte(&cl->reliablebuf, PRINT_HIGH);
				MSG_WriteString(&cl->reliablebuf, ErrorStr.c_str());
			}

			Printf(PRINT_HIGH, "%s is banned and unable to join! (reason: %s)\n", NET_AdrToString (net_from), BanList[i].Reason.c_str());

			SV_SendPacket (players[n]);
			cl->displaydisconnect = false;
			return true;
		}
		else if (exception)	// don't bother because they'll be allowed multiple times
		{
			std::string BanStr;
			BanStr += "\nBan Exception (reason: ";
			//if (*(WhiteList[i].Reason.c_str()))
				BanStr += WhiteList[i].Reason;
			//else
			//	BanStr += "none given";
			//BanStr += ")\n\n";

			MSG_WriteMarker(&cl->reliablebuf, svc_print);
			MSG_WriteByte(&cl->reliablebuf, PRINT_HIGH);
			MSG_WriteString(&cl->reliablebuf, BanStr.c_str());

			return false;
		}
	}

	return false;
}*/

/*BEGIN_COMMAND(addban) {
	std::string reason;

	if (argc < 2)
		return;

	if (argc >= 3)
		reason = BuildString(argc - 2, (const char **)(argv + 2));
	else
		reason = "none given";

	std::string IPtoBan = BuildString(argc - 1, (const char **)(argv + 1));
	SV_IPListAdd(&BanList, "Ban", IPtoBan, reason);
	} END_COMMAND(addban)*/

 /*BEGIN_COMMAND(delban) {
	if (argc < 2)
		return;

	std::string IPtoBan = BuildString(argc - 1, (const char **)(argv + 1));
	SV_IPListDelete(&BanList, "Ban", IPtoBan);
	} END_COMMAND(delban)*/

/* BEGIN_COMMAND(banlist) {
	SV_IPListDisplay(&BanList, "Ban");
	} END_COMMAND(banlist) */

BEGIN_COMMAND(clearbans) {
	SV_IPListClear(&BanList, "Ban");
} END_COMMAND(clearbans)

BEGIN_COMMAND(addexception) {
	std::string reason;

	if (argc < 2)
		return;

	if (argc >= 3)
		reason = BuildString(argc - 2, (const char **)(argv + 2));
	else
		reason = "none given";

	std::string IPtoBan = BuildString(argc - 1, (const char **)(argv + 1));
	SV_IPListAdd(&WhiteList, "Exception", IPtoBan, reason);
} END_COMMAND(addexception)

BEGIN_COMMAND(delexception) {
	if (argc < 2)
		return;

	std::string IPtoBan = BuildString(argc - 1, (const char **)(argv + 1));
	SV_IPListDelete(&WhiteList, "Exception", IPtoBan);
} END_COMMAND(delexception)

BEGIN_COMMAND(exceptionlist) {
	SV_IPListDisplay(&WhiteList, "Exception");
} END_COMMAND(exceptionlist)

BEGIN_COMMAND(clearexceptions) {
	SV_IPListClear(&WhiteList, "Exception");
} END_COMMAND(clearexceptions)

// Nes - Same as kick, only add ban.
BEGIN_COMMAND(kickban) {
	if (argc < 2)
		return;

	player_t &player = idplayer(atoi(argv[1]));
	std::string command, tempipstring;
	short tempip[IPADDRSIZE];

	// Check for validity...
	if (!validplayer(player)) {
		Printf(PRINT_HIGH, "bad client number: %d\n", atoi(argv[1]));
		return;
	}

	if (!player.ingame()) {
		Printf(PRINT_HIGH, "client %d not in game\n", atoi(argv[1]));
		return;
	}

	// Generate IP for the ban...
	for (int i = 0; i < IPADDRSIZE; i++)
		tempip[i] = (short)player.client.address.ip[i];

	SV_BanStringify(&tempipstring, tempip);

	// The kick...
	if (argc > 2) {
		std::string reason = BuildString(argc - 2, (const char **)(argv + 2));
		SV_BroadcastPrintf(PRINT_HIGH, "%s was kickbanned from the server! (Reason: %s)\n", player.userinfo.netname, reason.c_str());
	} else {
		SV_BroadcastPrintf(PRINT_HIGH, "%s was kickbanned from the server!\n", player.userinfo.netname);
	}

	player.client.displaydisconnect = false;
	SV_DropClient(player);

	// ... and the ban!
	command = "addban ";
	command += tempipstring;
	if (argc > 2) {
		command += " ";
		command += BuildString(argc - 2, (const char **)(argv + 2));
	}
	AddCommandString(command);
} END_COMMAND(kickban)
