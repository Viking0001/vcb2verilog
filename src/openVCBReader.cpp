#include <algorithm>
#include <cstring>
// Code for file reading

#include "openVCB.h"

#include <zstd.h>
#include <vector>
#include <stdio.h>
#include <fstream>
#include <istream>
#include <sstream>
#include <iterator>

namespace openVCB {
	const int colorPallet[] = {
		// Off
		0x000000,

		0x5a5845,
		0x2e475d,
		0x4d383e,
		0x66788e,

		0x3e4d3e,
		0x384b56,
		0x4d392f,

		0x4d3744,
		0x1a3c56,
		0x4d453e,

		0x433d56,
		0x3b2854,

		0x4d243c,
		0x384d47,
		0x323841,
		0x6c9799,

		0xa1ab8c,
		0x3f4b5b,

		// On
		0x000000,

		0xa19856,
		0x63b1ff,
		0xff5e5e,
		0x66788e,

		0x92ff63,
		0x63f2ff,
		0xffa200,

		0xff628a,
		0x30d9ff,
		0xffc663,

		0xae74ff,
		0xa600ff,

		0xff0041,
		0x63ff9f,
		0xffffff,
		0xb2fbfe,

		0xa1ab8c,
		0x3f4b5b
	};

	const char* inkNames[] = {
		"None",

		"Trace (Off)",
		"Read (Off)",
		"Write (Off)",
		"Cross",

		"Buffer (Off)",
		"OR (Off)",
		"NAND (Off)",

		"NOT (Off)",
		"NOR (Off)",
		"AND (Off)",

		"XOR (Off)",
		"NXOR (Off)",

		"Clock (Off)",
		"Latch (Off)",
		"LED (Off)",
		"Bundle (Off)",

		"Filler",
		"Annotation",

		"UNDEFINED",

		"Trace (On)",
		"Read (On)",
		"Write (On)",
		"UNDEFINED",

		"Buffer (On)",
		"OR (On)",
		"NAND (On)",

		"NOT (On)",
		"NOR (On)",
		"AND (On)",

		"XOR (On)",
		"NXOR (On)",

		"Clock (On)",
		"Latch (On)",
		"LED (On)",
		"Bundle (On)",

		"UNDEFINED",
		"UNDEFINED"
	};

	const char* getInkString(Ink ink) {
		int i = (int)ink;
		if (i >= 128) i += (int)Ink::numTypes - 128;
		if (i < 0 || i > 2 * (int)Ink::numTypes) return "None";
		return inkNames[i];
	}

	inline int col2int(int col) {
		int r = col & 0xff0000;
		int g = col & 0xff00;
		int b = col & 0xff;
		return (r >> 16) | g | (b << 16);
	}

	int traceColors[] = {
			0x2a3541,
			0x9fa8ae,
			0xa1555e,
			0xa16c56,
			0xa18556,
			0xa19856,
			0x99a156,
			0x88a156,
			0x6ca156,
			0x56a18d,
			0x5693a1,
			0x567ba1,
			0x5662a1,
			0x6656a1,
			0x8756a1,
			0xa15597
	};

	InkPixel color2ink(int col) {
		col = col2int(col);
		InkPixel pix = {};
		for (int i = 0; i < 16; i++)
			if (col == traceColors[i]) {
				pix.meta = (int16_t)i;
				pix.ink = (int16_t)Ink::Trace;
				break;
			}

		switch (col) {
		case 0x3a4551:
			pix.ink = (int16_t)Ink::Annotation;
			break;

		case 0x8caba1:
			pix.ink = (int16_t)Ink::Filler;
			break;

		default:
			for (int i = 0; i < 2 * (int)Ink::numTypes; i++)
				if (colorPallet[i] == col) {
					if (i >= (int)Ink::numTypes)
						i += 128 - (int)Ink::numTypes;
					pix.ink = (int16_t)i;
					break;
				}
		}

		return pix;
	}

	std::string split(std::string data, const char* t, int& start) {
		int tlen = strlen(t);
		int begin = start;
		int end = data.find(t, start + 1);

		if (end < 0) {
			throw std::runtime_error(std::string("Missing expected token in .vcb file: [") + t + "]");
		}

		start = end + tlen;
		return data.substr(begin, end - begin);
	}

	bool processData(std::vector<unsigned char> logicData, int headerSize, int& width, int& height, unsigned char*& originalImage, unsigned long long& imSize) {
		int* header = (int*)&logicData[logicData.size() - headerSize];

		const int imgDSize = header[5];
		width = header[3];
		height = header[1];

		if (imgDSize != width * height * 4) {
			fprintf(stderr, "Error: Header width x height does not match header length\n");
			return 0;
		}

		unsigned char* cc = &logicData[0];
		size_t ccSize = logicData.size() - headerSize;

		imSize = ZSTD_getFrameContentSize(cc, ccSize);

		if (imSize == ZSTD_CONTENTSIZE_ERROR) {
			fprintf(stderr, "Error: Data not compressed by zstd!\n");
			return 0;
		}
		else if (imSize == ZSTD_CONTENTSIZE_UNKNOWN) {
			fprintf(stderr, "Error: Original size unknown!\n");
			return 0;
		}
		else if (imSize != imgDSize) {
			fprintf(stderr, "Error: Decompressed image data size does not match header\n");
			return 0;
		}
		else {
			originalImage = new unsigned char[imSize];
			size_t const dSize = ZSTD_decompress(originalImage, imSize, cc, ccSize);
			return 1;
		}
	}

	bool Project::processLogicData(std::vector<unsigned char> logicData, int headerSize) {
		unsigned long long imSize;
		if (processData(logicData, headerSize, width, height, originalImage, imSize)) {
			image = new InkPixel[imSize];
#pragma omp parallel for schedule(static, 8196)
			for (int i = 0; i < imSize / 4; i++)
				image[i] = color2ink(((int*)originalImage)[i]);

			return 1;
		}

		return 0;
	}

	void Project::Project::processDecorationData(std::vector<unsigned char> decorationData, int*& decoData) {
		unsigned long long imSize;
		int width, height;
		if (processData(decorationData, 24, width, height, (unsigned char*&)decoData, imSize)) {
			for (int i = 0; i < imSize / 4; i++) {
				int color = decoData[i];
				decoData[i] = col2int(color);
			}
		}
	}

	bool Project::readFromVCB(std::string filePath) {
		std::ifstream stream(filePath, std::ios::binary);
		if (!stream.is_open()) {
			fprintf(stderr, "Error: Cannot open file for reading: \"%s\"\n", filePath.c_str());
			return false;
		}

		std::stringstream ss;
		ss << stream.rdbuf();
		std::string content = ss.str();
		stream.close();

		if (content.empty()) {
			fprintf(stderr, "Error: File \"%s\" is empty\n", filePath.c_str());
			return false;
		}

		bool isGodot = (content.find("PoolByteArray") != std::string::npos ||
		                content.find("[gd_resource") != std::string::npos);

		try {
			if (isGodot) {
				return readFromGodotVCB(content);
			} else {
				return readFromJsonVCB(content);
			}
		} catch (const std::exception& e) {
			fprintf(stderr, "Error parsing VCB file \"%s\": %s\n", filePath.c_str(), e.what());
			return false;
		}
	}

	bool Project::readFromJsonVCB(const std::string& json) {
		// 1. assembly
		size_t pos = json.find("\"assembly\": \"");
		if (pos != std::string::npos) {
			pos += 13;
			size_t end = json.find("\",", pos);
			if (end != std::string::npos) {
				assembly = json.substr(pos, end - pos);
			}
		}

		// 2. is_vmem_enabled
		bool vmemFlag = false;
		pos = json.find("\"is_vmem_enabled\":");
		if (pos != std::string::npos) {
			size_t valStart = json.find_first_not_of(" \t\r\n", pos + 18);
			if (valStart != std::string::npos && json.substr(valStart, 4) == "true") {
				vmemFlag = true;
			}
		}

		// 3. layers (base64 zstd streams)
		pos = json.find("\"layers\":");
		if (pos == std::string::npos) {
			throw std::runtime_error("Missing \"layers\" key in JSON .vcb file");
		}
		pos = json.find("[", pos);
		std::vector<std::string> b64Layers;
		while (pos != std::string::npos) {
			size_t strStart = json.find("\"", pos + 1);
			if (strStart == std::string::npos) break;
			size_t nextBracket = json.find("]", pos + 1);
			if (nextBracket != std::string::npos && strStart > nextBracket) break;
			size_t strEnd = json.find("\"", strStart + 1);
			if (strEnd == std::string::npos) break;
			b64Layers.push_back(json.substr(strStart + 1, strEnd - strStart - 1));
			pos = strEnd;
		}

		if (b64Layers.empty()) {
			throw std::runtime_error("No layers found in JSON .vcb file");
		}

		// Layer 0 is logic data
		std::vector<unsigned char> logicData = b64decode(b64Layers[0]);

		// Layers 1..3 are decoration data
		for (size_t i = 1; i < b64Layers.size() && i <= 3; i++) {
			std::vector<unsigned char> decoData = b64decode(b64Layers[i]);
			processDecorationData(decoData, decoration[i - 1]);
		}

		// 4. led_palette
		pos = json.find("\"led_palette\":");
		if (pos != std::string::npos) {
			size_t bStart = json.find("[", pos);
			size_t bEnd = json.find("]", bStart);
			if (bStart != std::string::npos && bEnd != std::string::npos) {
				std::string palStr = json.substr(bStart + 1, bEnd - bStart - 1);
				std::stringstream s(palStr);
				std::string item;
				int idx = 0;
				while (std::getline(s, item, ',') && idx < 16) {
					item.erase(std::remove(item.begin(), item.end(), '\"'), item.end());
					item.erase(std::remove_if(item.begin(), item.end(), ::isspace), item.end());
					if (!item.empty()) {
						ledPalette[idx++] = std::stoul(item, nullptr, 16);
					}
				}
			}
		}

		// 5. vmem_settings
		pos = json.find("\"vmem_settings\":");
		int vmemArr[14] = {0};
		if (pos != std::string::npos) {
			size_t bStart = json.find("[", pos);
			size_t bEnd = json.find("]", bStart);
			if (bStart != std::string::npos && bEnd != std::string::npos) {
				std::string arrStr = json.substr(bStart + 1, bEnd - bStart - 1);
				std::stringstream s(arrStr);
				std::string item;
				for (int i = 0; i < 14 && std::getline(s, item, ','); i++) {
					vmemArr[i] = std::atoi(item.c_str());
				}
			}
		}

		// Set the vmem settings
		vmAddr.numBits = std::max(0, std::min(vmemArr[0], 32));
		vmAddr.pos.x = vmemArr[1];
		vmAddr.pos.y = vmemArr[2];
		vmAddr.stride.x = -vmemArr[3];
		vmAddr.stride.y = vmemArr[4];
		vmAddr.size.x = vmemArr[5];
		vmAddr.size.y = vmemArr[6];

		vmData.numBits = std::max(0, std::min(vmemArr[7], 32));
		vmData.pos.x = vmemArr[8];
		vmData.pos.y = vmemArr[9];
		vmData.stride.x = -vmemArr[10];
		vmData.stride.y = vmemArr[11];
		vmData.size.x = vmemArr[12];
		vmData.size.y = vmemArr[13];

		if (vmemFlag) {
			vmemSize = 1ull << vmAddr.numBits;
			vmem = new int[vmemSize];
			memset(vmem, 0, 4 * vmemSize);
		}

		if (!Project::processLogicData(logicData, 24)) {
			return false;
		}

		if (vmemFlag) {
			for (int i = 0; i < vmAddr.numBits; i++) {
				auto start = vmAddr.pos + i * vmAddr.stride;
				auto end = start + vmAddr.size;
				for (auto p = start; p.x < end.x; p.x++) {
					for (p.y = start.y; p.y < end.y; p.y++) {
						if (p.x < 0 || p.x >= width || p.y < 0 || p.y >= height)
							continue;
						image[p.x + p.y * width].ink = (int16_t)Ink::LatchOff;
					}
				}
			}

			for (int i = 0; i < vmData.numBits; i++) {
				auto start = vmData.pos + i * vmData.stride;
				auto end = start + vmData.size;
				for (auto p = start; p.x < end.x; p.x++) {
					for (p.y = start.y; p.y < end.y; p.y++) {
						if (p.x < 0 || p.x >= width || p.y < 0 || p.y >= height)
							continue;
						image[p.x + p.y * width].ink = (int16_t)Ink::LatchOff;
					}
				}
			}
		}

		return true;
	}

	bool Project::readFromGodotVCB(const std::string& godotObj) {
		std::string obj = godotObj;

		// split out assembly
		int pos = 0;
		split(obj, "\"assembly\": \"", pos);
		assembly = split(obj, "\",", pos);

		// if VMem is enabled
		split(obj, "\"is_vmem_enabled\": ", pos);
		auto vmemFlag = split(obj, ",", pos) == "true";

		std::vector<unsigned char> logicData;
		split(obj, "PoolByteArray(", pos);

		{
			auto dat = split(obj, " ), PoolByteArray( ", pos);
			std::stringstream s(dat);
			std::string val;
			while (std::getline(s, val, ','))
				logicData.push_back(atoi(val.c_str() + 1));
		}

		std::vector<unsigned char> decorationData[3];
		{
			pos--;
			auto dat = split(obj, " ), PoolByteArray( ", pos);
			std::stringstream s(dat);
			std::string val;
			while (std::getline(s, val, ','))
				decorationData[0].push_back(atoi(val.c_str() + 1));
		}

		{
			pos--;
			auto dat = split(obj, " ), PoolByteArray( ", pos);
			std::stringstream s(dat);
			std::string val;
			while (std::getline(s, val, ','))
				decorationData[1].push_back(atoi(val.c_str() + 1));
		}

		{
			pos--;
			auto dat = split(obj, " ) ]", pos);
			std::stringstream s(dat);
			std::string val;
			while (std::getline(s, val, ','))
				decorationData[2].push_back(atoi(val.c_str() + 1));
		}

		//led palette
		std::vector<int> vecPalette;
		split(obj, "\"led_palette\": [ ", pos);

		auto dat = split(obj, " ]", pos);
		std::stringstream s(dat);
		std::string val;
		while (std::getline(s, val, ',')) {
			val.erase(remove(val.begin(), val.end(), '\"'), val.end());
			vecPalette.push_back(std::stoul(val, nullptr, 16));
		}

		for (int i = 0; i < std::min((int)vecPalette.size(), 16) ; i++) {
			ledPalette[i] = vecPalette[i];
		}

		split(obj, "\"vmem_settings\": [ ", pos);

		// Get VMem settings
		int vmemArr[14];
		{
			auto dat = split(obj, " ]", pos);
			std::stringstream s(dat);
			std::string val;
			for (size_t i = 0; i < 14; i++) {
				std::getline(s, val, ',');
				vmemArr[i] = atoi(val.c_str());
			}
		}

		// Set the vmem settings
		vmAddr.numBits = std::max(0, std::min(vmemArr[0], 32));
		vmAddr.pos.x = vmemArr[1];
		vmAddr.pos.y = vmemArr[2];
		vmAddr.stride.x = -vmemArr[3];
		vmAddr.stride.y = vmemArr[4];
		vmAddr.size.x = vmemArr[5];
		vmAddr.size.y = vmemArr[6];

		vmData.numBits = std::max(0, std::min(vmemArr[7], 32));
		vmData.pos.x = vmemArr[8];
		vmData.pos.y = vmemArr[9];
		vmData.stride.x = -vmemArr[10];
		vmData.stride.y = vmemArr[11];
		vmData.size.x = vmemArr[12];
		vmData.size.y = vmemArr[13];

		if (vmemFlag) {
			vmemSize = 1ull << vmAddr.numBits;
			vmem = new int[vmemSize];
			memset(vmem, 0, 4 * vmemSize);
		}

		if (!Project::processLogicData(logicData, 24)) {
			return false;
		}

		if (vmemFlag) {
			for (int i = 0; i < vmAddr.numBits; i++) {
				auto start = vmAddr.pos + i * vmAddr.stride;
				auto end = start + vmAddr.size;
				for (auto p = start; p.x < end.x; p.x++) {
					for (p.y = start.y; p.y < end.y; p.y++) {
						if (p.x < 0 || p.x >= width || p.y < 0 || p.y >= height)
							continue;
						image[p.x + p.y * width].ink = (int16_t)Ink::LatchOff;
					}
				}
			}

			for (int i = 0; i < vmData.numBits; i++) {
				auto start = vmData.pos + i * vmData.stride;
				auto end = start + vmData.size;
				for (auto p = start; p.x < end.x; p.x++) {
					for (p.y = start.y; p.y < end.y; p.y++) {
						if (p.x < 0 || p.x >= width || p.y < 0 || p.y >= height)
							continue;
						image[p.x + p.y * width].ink = (int16_t)Ink::LatchOff;
					}
				}
			}
		}

		Project::processDecorationData(decorationData[0], decoration[0]);
		Project::processDecorationData(decorationData[1], decoration[1]);
		Project::processDecorationData(decorationData[2], decoration[2]);
		return true;
	}
}