#ifndef __CL_DEMO_H__
#define __CL_DEMO_H__

#include "doomtype.h"
#include "d_net.h"
#include <string>
#include <vector>
#include <list>

class NetDemo
{
public:
	NetDemo();
	~NetDemo();
	NetDemo(const NetDemo &rhs);
	NetDemo& operator=(const NetDemo &rhs);
	
	bool startPlaying(const std::string &filename);
	bool startRecording(const std::string &filename);
	bool stopPlaying();
	bool stopRecording();
	bool pause();
	bool resume();
	
	void writeMessages();
	void readMessages(buf_t* netbuffer);
	void capture(const buf_t* netbuffer);

	bool isRecording() const { return (state == NetDemo::st_recording); }
	bool isPlaying() const { return (state == NetDemo::st_playing); }
	bool isPaused() const { return (state == NetDemo::st_paused); }
	
	int getSpacing() const { return header.snapshot_spacing; }
	
	void skipTo(buf_t *netbuffer, int ticnum);
private:
	typedef enum
	{
		st_stopped,
		st_recording,
		st_playing,
		st_paused
	} netdemo_state_t;

	typedef enum
	{
		msg_packet		= 0xAA,
		msg_snapshot
	} netdemo_message_t;

	void cleanUp();
	void copy(NetDemo &to, const NetDemo &from);
	void error(const std::string &message);
	void reset();

	int snapshotLookup(int ticnum);
	void writeLauncherSequence(buf_t *netbuffer);
	void writeConnectionSequence(buf_t *netbuffer);
	void writeSnapshot(buf_t *netbuffer);
	void writeSnapshotData(buf_t *netbuffer);
	void writeSnapshotIndexEntry();
	void readSnapshot(buf_t *netbuffer, size_t index);
	void writeChunk(const byte *data, size_t size, netdemo_message_t type);
	bool writeHeader();
	bool readHeader();
	bool writeIndex();
	bool readIndex();
	void writeLocalCmd(buf_t *netbuffer) const;
	bool readMessageHeader(netdemo_message_t &type, uint32_t &len, uint32_t &tic) const;
	void readMessageBody(buf_t *netbuffer, uint32_t len);
	void writeFullUpdate(int ticnum);

	int calculateTimeElapsed();
	int calculateTotalTime();

	typedef struct
	{
		uint32_t	ticnum;
		uint32_t	offset;			// offset in the demo file
	} netdemo_snapshot_entry_t;
	
	typedef struct
	{
		byte		type;
		uint32_t	length;
		uint32_t	gametic;
	} message_header_t;

	typedef struct
	{
		char		identifier[4];  		// "ODAD"
		byte		version;
		byte    	compression;    		// type of compression used
		uint32_t	snapshot_index_offset;	// offset from start of the file for the index
		uint32_t	snapshot_index_size;	// gametic filepos index follows header
		uint16_t	snapshot_spacing;		// number of gametics between indices
		uint32_t	starting_gametic;		// the gametic the demo starts at
		uint32_t	ending_gametic;			// the last gametic of the demo
		byte		reserved[40];   		// for future use
	} netdemo_header_t;
	
	static const size_t HEADER_SIZE = 64;
	static const size_t MESSAGE_HEADER_SIZE = 9;
	static const size_t INDEX_ENTRY_SIZE = 8;

	static const uint16_t SNAPSHOT_SPACING = 5 * TICRATE;

	static const size_t MAX_SNAPSHOT_SIZE = MAX_UDP_PACKET;
	
	netdemo_state_t		state;
	netdemo_state_t		oldstate;	// used when unpausing
	std::string			filename;
	FILE*				demofp;

	std::list<buf_t>	captured;

	netdemo_header_t	header;	
	std::vector<netdemo_snapshot_entry_t> snapshot_index;
};




#endif

