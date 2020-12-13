#include "ncmcrypt.h"
#include "aes.h"
#include "base64.h"
#include "cJSON.h"
#include "spdlog/spdlog.h"

#include <taglib/mpegfile.h>
#include <taglib/flacfile.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/id3v2tag.h>
#include <taglib/tag.h>

#include <filesystem>
#include <stdexcept>
#include <string>


const unsigned char S_CORE_KEY[17] = {
        0x68, 0x7A, 0x48, 0x52, 0x41, 0x6D, 0x73, 0x6F, 0x35, 0x6B, 0x49, 0x6E, 0x62, 0x61, 0x78, 0x57, 0
};
const unsigned char S_MODIFY_KEY[17] = {
        0x23, 0x31, 0x34, 0x6C, 0x6A, 0x6B, 0x5F, 0x21, 0x5C, 0x5D, 0x26, 0x30, 0x55, 0x3C, 0x27, 0x28, 0
};
const unsigned char PNG_FILE_HEAD[8] = {
        0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A
};

static void aesEcbDecrypt(const unsigned char *key, std::string &src, std::string &dst) {
    int n, i;

    unsigned char out[16];

    n = src.length() >> 4;

    dst.clear();

    AES aes(key);

    for (i = 0; i < n - 1; i++) {
        aes.decrypt((unsigned char *) src.c_str() + (i << 4), out);
        dst += std::string((char *) out, 16);
    }

    aes.decrypt((unsigned char *) src.c_str() + (i << 4), out);
    char pad = out[15];
    if (pad > 16) {
        pad = 0;
    }
    dst += std::string((char *) out, 16 - pad);
}


static std::string replaceFileExtension(const std::string &str, const std::string &extension) {
    std::filesystem::path path = str;
    path.replace_extension("." + extension);
    return path.string();
}

NeteaseMusicMetadata::~NeteaseMusicMetadata() {
    cJSON_Delete(mRaw);
}

NeteaseMusicMetadata::NeteaseMusicMetadata(cJSON *raw) {
    if (!raw) {
        return;
    }

    cJSON *swap;
    int artistLen, i;

    mRaw = raw;

    swap = cJSON_GetObjectItem(raw, "musicName");
    if (swap) {
        mName = std::string(cJSON_GetStringValue(swap));
    }

    swap = cJSON_GetObjectItem(raw, "album");
    if (swap) {
        mAlbum = std::string(cJSON_GetStringValue(swap));
    }

    swap = cJSON_GetObjectItem(raw, "artist");
    if (swap) {
        artistLen = cJSON_GetArraySize(swap);

        i = 0;
        for (i = 0; i < artistLen - 1; i++) {
            mArtist += std::string(cJSON_GetStringValue(cJSON_GetArrayItem(cJSON_GetArrayItem(swap, i), 0)));
            mArtist += "/";
        }
        mArtist += std::string(cJSON_GetStringValue(cJSON_GetArrayItem(cJSON_GetArrayItem(swap, i), 0)));
    }

    swap = cJSON_GetObjectItem(raw, "bitrate");
    if (swap) {
        mBitrate = swap->valueint;
    }

    swap = cJSON_GetObjectItem(raw, "duration");
    if (swap) {
        mDuration = swap->valueint;
    }

    swap = cJSON_GetObjectItem(raw, "format");
    if (swap) {
        mFormat = std::string(cJSON_GetStringValue(swap));
    }
}


bool NeteaseCrypt::isNcmFile() {

    unsigned int header;
    // TODO seek file to a specified position
    // TODO Compare with memcmp
    mFile.read(reinterpret_cast<char *>(&header), sizeof(header));
    if (header != (unsigned int) 0x4e455443) {
        return false;
    }

    mFile.read(reinterpret_cast<char *>(&header), sizeof(header));
    if (header != (unsigned int) 0x4d414446) {
        return false;
    }

    return true;
}

int NeteaseCrypt::read(char *s, std::streamsize n) {
    mFile.read(s, n);

    int gcount = mFile.gcount();

    if (gcount <= 0) {
        throw std::invalid_argument("can't read file");
    }

    return gcount;
}

void NeteaseCrypt::buildKeyBox(const unsigned char *key, unsigned int keyLen) {
    int i;
    for (i = 0; i < 256; ++i) {
        mKeyBox[i] = (unsigned char) i;
    }

    unsigned char swap = 0;
    unsigned char c = 0;
    unsigned char last_byte = 0;
    unsigned char key_offset = 0;

    for (i = 0; i < 256; ++i) {
        swap = mKeyBox[i];
        c = ((swap + last_byte + key[key_offset++]) & 0xff);
        if (key_offset >= keyLen) key_offset = 0;
        mKeyBox[i] = mKeyBox[c];
        mKeyBox[c] = swap;
        last_byte = c;
    }
}

std::string NeteaseCrypt::AlbumMIMEType(std::string &data) {
    if (memcmp(data.c_str(), PNG_FILE_HEAD, 8) == 0) {
        return std::string("image/png");
    }
    //TODO JPEG data should start with 0xFFD8 and end with 0xFFD9
    return std::string("image/jpeg");
}

void NeteaseCrypt::FixMetadata() {
    if (mDumpFilepath.length() <= 0) {
        throw std::invalid_argument("must dump before");
    }

    TagLib::File *audioFile;
    TagLib::Tag *tag;
    TagLib::ByteVector vector(mImageData.c_str(), mImageData.length());

    if (mFormat == NeteaseCrypt::MP3) {
        audioFile = new TagLib::MPEG::File(mDumpFilepath.c_str());
        tag = dynamic_cast<TagLib::MPEG::File *>(audioFile)->ID3v2Tag(true);

        if (mImageData.length() > 0) {
            TagLib::ID3v2::AttachedPictureFrame *frame = new TagLib::ID3v2::AttachedPictureFrame;

            frame->setMimeType(AlbumMIMEType(mImageData));
            frame->setPicture(vector);

            dynamic_cast<TagLib::ID3v2::Tag *>(tag)->addFrame(frame);
        }
    } else if (mFormat == NeteaseCrypt::FLAC) {
        audioFile = new TagLib::FLAC::File(mDumpFilepath.c_str());
        tag = audioFile->tag();

        if (mImageData.length() > 0) {
            TagLib::FLAC::Picture *cover = new TagLib::FLAC::Picture;
            cover->setMimeType(AlbumMIMEType(mImageData));
            cover->setType(TagLib::FLAC::Picture::FrontCover);
            cover->setData(vector);

            dynamic_cast<TagLib::FLAC::File *>(audioFile)->addPicture(cover);
        }
    }

    if (mMetaData != NULL) {
        tag->setTitle(TagLib::String(mMetaData->name(), TagLib::String::UTF8));
        tag->setArtist(TagLib::String(mMetaData->artist(), TagLib::String::UTF8));
        tag->setAlbum(TagLib::String(mMetaData->album(), TagLib::String::UTF8));
    }

    audioFile->save();
}

/**
 * Dump file to target location
 */
void NeteaseCrypt::Dump() {
    const int MAX_BUFFER_SIZE = 0x8000;
    unsigned char buffer[MAX_BUFFER_SIZE];
    std::ofstream output;
    while (!mFile.eof()) {
        int bufferSize = read((char *) buffer, MAX_BUFFER_SIZE);

        for (int i = 0; i < bufferSize; i++) {
            int j = (i + 1) & 0xff;
            buffer[i] ^= mKeyBox[(mKeyBox[j] + mKeyBox[(mKeyBox[j] + j) & 0xff]) & 0xff];
        }

        // Initialize ofstream object in 1st iteration
        if (!output.is_open()) {
            // identify format
            // ID3 format mp3
            if (buffer[0] == 0x49 && buffer[1] == 0x44 && buffer[2] == 0x33) {
                mDumpFilepath = replaceFileExtension(mFilepath, "mp3");
                mFormat = NeteaseCrypt::MP3;
            } else {
                mDumpFilepath = replaceFileExtension(mFilepath, "flac");
                mFormat = NeteaseCrypt::FLAC;
            }
            output.open(mDumpFilepath, std::ofstream::out | std::ofstream::binary);
        }
        output.write((char *) buffer, bufferSize);
    }

    output.flush();
    output.close();
}

NeteaseCrypt::~NeteaseCrypt() {
    delete mMetaData;
    if (mFile.is_open()) mFile.close();
}

NeteaseCrypt::NeteaseCrypt(std::string const &path) {
    // open file
    mFile.open(path, std::ios::in | std::ios::binary);

    if (!isNcmFile()) {
        throw std::invalid_argument("not netease protected file");
    }

    if (!mFile.seekg(2, std::ios_base::cur)) {
        throw std::invalid_argument("can't seek file");
    }

    mFilepath = path;


    // Read the size of the keyData
    unsigned int n;
    read(reinterpret_cast<char *>(&n), sizeof(unsigned int));
    if (n <= 0) throw std::invalid_argument("broken ncm file");

    char keyData[n];
    read(keyData, n);

    for (int i = 0; i < n; i++) {
        keyData[i] ^= 0x64;
    }

    std::string rawKeyData(keyData, n);
    std::string mKeyData;

    aesEcbDecrypt(S_CORE_KEY, rawKeyData, mKeyData);

    buildKeyBox((unsigned char *) mKeyData.c_str() + 17, mKeyData.length() - 17);

    read(reinterpret_cast<char *>(&n), sizeof(n));

    if (n <= 0) {
        spdlog::warn("Metadata information is missed in {}", path);
        mMetaData = NULL;
    } else {
        char modifyData[n];
        read(modifyData, n);

        for (int i = 0; i < n; i++) {
            modifyData[i] ^= 0x63;
        }

        std::string swapModifyData;
        std::string modifyOutData;
        std::string modifyDecryptData;

        swapModifyData = std::string(modifyData + 22, n - 22);

        // escape `163 key(Don't modify):`
        Base64::Decode(swapModifyData, modifyOutData);

        aesEcbDecrypt(S_MODIFY_KEY, modifyOutData, modifyDecryptData);

        // escape `music:`
        modifyDecryptData = std::string(modifyDecryptData.begin() + 6, modifyDecryptData.end());

        mMetaData = new NeteaseMusicMetadata(cJSON_Parse(modifyDecryptData.c_str()));
    }

    // skip crc32 & unused charset
    if (!mFile.seekg(9, mFile.cur)) {
        throw std::invalid_argument("can't seek file");
    }

    read(reinterpret_cast<char *>(&n), sizeof(n));

    if (n > 0) {
        char *imageData = (char *) malloc(n);
        read(imageData, n);
        mImageData = std::string(imageData, n);
    } else {
        spdlog::warn("Can't fix album image for file {} caused by missing album data!", path);
    }
}
