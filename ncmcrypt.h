#pragma once

#include "aes.h"
#include "cJSON.h"

#include <iostream>
#include <fstream>

class NeteaseMusicMetadata {

private:
	std::string mAlbum;
	std::string mArtist;
	std::string mFormat;
	std::string mName;
	int mDuration;
	int mBitrate;

private:
	cJSON* mRaw;

public:
	NeteaseMusicMetadata(cJSON*);
	~NeteaseMusicMetadata();
    const std::string& name() const { return mName; }
    const std::string& album() const { return mAlbum; }
    const std::string& artist() const { return mArtist; }
    const std::string& format() const { return mFormat; }
    const int duration() const { return mDuration; }
    const int bitrate() const { return mBitrate; }

};

class NeteaseCrypt {

private:
	enum NcmFormat { MP3, FLAC };

private:
	std::string mFilepath;
	std::string mDumpFilepath;
	NcmFormat mFormat;
	std::string mImageData;
	std::ifstream mFile;
	unsigned char mKeyBox[256];
	NeteaseMusicMetadata* mMetaData;

private:
	bool isNcmFile();
	int read(char *s, std::streamsize n);
	void buildKeyBox(const unsigned char *key, unsigned int keyLen);
	static std::string AlbumMIMEType(std::string& data);

//public:
//	const std::string& filepath() const { return mFilepath; }
//	const std::string& dumpFilepath() const { return mDumpFilepath; }

public:
	explicit NeteaseCrypt(std::string const&);
	~NeteaseCrypt();

public:
	void Dump();
	void FixMetadata();
};