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

#include <curl/curl.h>

#include "c_dispatch.h"
#include "cmdlib.h"
#include "d_player.h"
#include "sv_banlist.h"
#include "sv_main.h"

Banlist banlist;

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
		if (!(this->ip[i] == address.ip[i] || mask[i] == true)) {
			return false;
		}
	}

	return true;
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

BEGIN_COMMAND (testrange) {
	IPRange ip;
	if (ip.set("192.168.*.*")) {
		Printf(PRINT_HIGH, "%s\n", ip.string().c_str());

		netadr_t address;
		address.ip[0] = 192;
		address.ip[1] = 169;
		address.ip[2] = 0;
		address.ip[3] = 1;

		if (ip.check(address)) {
			Printf(PRINT_HIGH, "Yes!\n");
		} else {
			Printf(PRINT_HIGH, "No!\n");
		}
	}
} END_COMMAND (testrange)

bool Banlist::add(std::string address) { return true; }
void Banlist::check(std::string address) { }
void Banlist::debug() { }
void Banlist::list() { }
void Banlist::remove(std::string address) { }

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
bool SV_BanCheck (client_t *cl, int n)
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
}

BEGIN_COMMAND(addban) {
	std::string reason;

	if (argc < 2)
		return;

	if (argc >= 3)
		reason = BuildString(argc - 2, (const char **)(argv + 2));
	else
		reason = "none given";

	std::string IPtoBan = BuildString(argc - 1, (const char **)(argv + 1));
	SV_IPListAdd(&BanList, "Ban", IPtoBan, reason);
} END_COMMAND(addban)

BEGIN_COMMAND(delban) {
	if (argc < 2)
		return;

	std::string IPtoBan = BuildString(argc - 1, (const char **)(argv + 1));
	SV_IPListDelete(&BanList, "Ban", IPtoBan);
} END_COMMAND(delban)

BEGIN_COMMAND(banlist) {
	SV_IPListDisplay(&BanList, "Ban");
} END_COMMAND(banlist)

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
