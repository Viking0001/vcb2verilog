#pragma once
/*
 * openVCB - Virtual Circuit Board parser and circuit data structures
 * Adapted for vcb2verilog CLI converter.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>

namespace openVCB {
	struct LatchInterface {
		glm::ivec2 pos;
		glm::ivec2 stride;
		glm::ivec2 size;
		int numBits;
		int gids[64];
	};

	/// <summary>
	/// 8 bit state.
	/// 7 bit type + 1 active bit
	/// </summary>
	enum class Logic {
		NonZeroOff,
		ZeroOff,
		XorOff,
		XnorOff,
		LatchOff,
		ClockOff,

		numTypes,

		NonZero = 128,
		Zero,
		Xor,
		Xnor,
		Latch,
		Clock
	};

	/// <summary>
	/// 8 bit state.
	/// 7 bit type + 1 active bit
	/// </summary>
	enum class Ink {
		None = 0,

		TraceOff = 1,
		ReadOff,
		WriteOff,
		Cross,

		BufferOff,
		OrOff,
		NandOff,

		NotOff,
		NorOff,
		AndOff,

		XorOff,
		XnorOff,

		ClockOff,
		LatchOff,
		LedOff,
		BundleOff,

		Filler,
		Annotation,

		numTypes,

		Trace = 129,
		Read,
		Write,
		InvalidCross,

		Buffer,
		Or,
		Nand,

		Not,
		Nor,
		And,

		Xor,
		Xnor,

		Clock,
		Latch,
		Led,
		Bundle,

		InvalidFiller,
		InvalidAnnotation
	};

	extern const int colorPallet[];
	extern const char* inkNames[];

	// Sets the ink type to be on or off
	inline Ink setOn(Ink ink, bool state) {
		return (Ink)(((int)ink & 0x7f) | (state << 7));
	}

	inline Ink setOn(Ink ink) {
		return (Ink)(((int)ink & 0x7f) | 128);
	}

	inline Ink setOff(Ink ink) {
		return (Ink)((int)ink & 0x7f);
	}

	inline bool getOn(Ink ink) {
		return (int)ink >> 7;
	}

	inline Logic setOn(Logic logic, bool state) {
		return (Logic)(((int)logic & 0x7f) | (state << 7));
	}

	inline Logic setOn(Logic logic) {
		return (Logic)(((int)logic & 0x7f) | 128);
	}

	inline Logic setOff(Logic logic) {
		return (Logic)((int)logic & 0x7f);
	}

	inline bool getOn(Logic logic) {
		return (int)logic >> 7;
	}

	// Gets the logic type of said ink
	inline Logic inkLogicType(Ink ink) {
		ink = setOff(ink);
		switch (ink) {
		case Ink::NotOff:
		case Ink::NorOff:
		case Ink::AndOff:
			return Logic::ZeroOff;

		case Ink::XorOff:
			return Logic::XorOff;

		case Ink::XnorOff:
			return Logic::XnorOff;

		case Ink::LatchOff:
			return Logic::LatchOff;

		case Ink::ClockOff:
			return Logic::ClockOff;
		}

		return Logic::NonZeroOff;
	}

	// Gets the string name of the ink
	const char* getInkString(Ink ink);

	// Base64 helper declared for blueprint and JSON VCB decoding
	std::vector<unsigned char> b64decode(std::string data);
	bool isBase64(std::string text);

	struct InkState {
		// Number of active inputs
		int16_t activeInputs;
		// Flags for traversal
		unsigned char visited;
		// Current logic state
		unsigned char logic;
	};

	struct SparseMat {
		// Size of the matrix
		int n;
		// Number of non-zero entries
		int nnz;
		// CSC sparse matrix ptr
		int* ptr;
		// CSC sparse matrix rows
		int* rows;
	};

	struct InkPixel {
		int16_t ink;
		int16_t meta;

		Ink getInk() {
			return (Ink)ink;
		}
	};

	class Project {
	public:
		int* vmem = nullptr; // null if vmem is not used
		size_t vmemSize = 0;
		std::string assembly;
		LatchInterface vmAddr;
		LatchInterface vmData;
		LatchInterface vmWe;
		uint32_t lastVMemAddr = 0;

		~Project();

		int height = 0;
		int width = 0;

		// An image containing component indices
		unsigned char* originalImage = nullptr;
		InkPixel* image = nullptr;
		int* indexImage = nullptr;
		int* decoration[3]{ nullptr, nullptr, nullptr }; // on / off / unknown
		int ledPalette[16]{
			0x323841, 0xffffff, 0xff0000, 0x00ff00, 0x0000ff, 0xff0000, 0x00ff00, 0x0000ff,
			0xff0000, 0x00ff00, 0x0000ff, 0xff0000, 0x00ff00, 0x0000ff, 0xff0000, 0x00ff00
		};
		int numGroups = 0;

		std::vector<int> clockGIDs;
		unsigned long long clockCounter = 0;
		unsigned long long clockPeriod = 2;

		// Adjacency matrix (CSC sparse format)
		SparseMat writeMap = {};
		// Stores the logic states of each group
		InkState* states = nullptr;
		// Stores the actual ink type of each group
		Ink* stateInks = nullptr;

		// Map of symbols during assembleVmem()
		std::unordered_map<std::string, long long> assemblySymbols;
		std::unordered_map<long long, long long> lineNumbers;

		// Builds a project from a VCB file (supports both Godot v1 and JSON v2 formats)
		bool readFromVCB(std::string p);
		bool readFromJsonVCB(const std::string& json);
		bool readFromGodotVCB(const std::string& godotObj);

		// Decode base64 data from clipboard, then process logic data
		bool readFromBlueprint(std::string clipboardData);

		// Decompress zstd logic data to an image
		bool processLogicData(std::vector<unsigned char> logicData, int headerSize);

		// Decompress zstd decoration data to an image
		void processDecorationData(std::vector<unsigned char> decorationData, int*& originalImage);

		// Samples the ink at a pixel. Returns ink and group id
		std::pair<Ink, int> sample(glm::ivec2 pos);

		// Assemble the vmem from this->assembly
		void assembleVmem(char* err = nullptr);

		// Dump vmem contents to file
		void dumpVMemToText(std::string p);

		// Toggles the latch at position / gid
		void toggleLatch(glm::ivec2 pos);
		void toggleLatch(int gid);

		// Preprocesses the image into the graph representation
		void preprocess(bool useGorder = false);
	};
}
