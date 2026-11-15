[33madfff3a[m[33m ([m[1;36mHEAD[m[33m -> [m[1;32mmain[m[33m, [m[1;31morigin/main[m[33m)[m Clean test artifacts
[1mdiff --git a/src/openVCB.h b/src/openVCB.h[m
[1mindex 9841a97..7bd9270 100644[m
[1m--- a/src/openVCB.h[m
[1m+++ b/src/openVCB.h[m
[36m@@ -75,7 +75,6 @@[m [mnamespace openVCB {[m
 		Annotation,[m
 [m
 		numTypes,[m
[31m-[m
 		Tunnel = 100,[m
 		Mesh = 101,[m
 [m
[1mdiff --git a/src/openVCBReader.cpp b/src/openVCBReader.cpp[m
[1mindex 1cf8c9d..d95d752 100644[m
[1m--- a/src/openVCBReader.cpp[m
[1m+++ b/src/openVCBReader.cpp[m
[36m@@ -13,423 +13,420 @@[m
 #include <iterator>[m
 [m
 namespace openVCB {[m
[31m-	const int colorPallet[] = {[m
[31m-		// Off[m
[31m-		0x000000,[m
[32m+[m[32m    const int colorPallet[] = {[m
[32m+[m[32m        // Off[m
[32m+[m[32m        0x000000,[m
 [m
[31m-		0x5a5845,[m
[31m-		0x2e475d,[m
[31m-		0x4d383e,[m
[31m-		0x66788e,[m
[32m+[m[32m        0x5a5845,[m
[32m+[m[32m        0x2e475d,[m
[32m+[m[32m        0x4d383e,[m
[32m+[m[32m        0x66788e,[m
 [m
[31m-		0x3e4d3e,[m
[31m-		0x384b56,[m
[31m-		0x4d392f,[m
[32m+[m[32m        0x3e4d3e,[m
[32m+[m[32m        0x384b56,[m
[32m+[m[32m        0x4d392f,[m
 [m
[31m-		0x4d3744,[m
[31m-		0x1a3c56,[m
[31m-		0x4d453e,[m
[32m+[m[32m        0x4d3744,[m
[32m+[m[32m        0x1a3c56,[m
[32m+[m[32m        0x4d453e,[m
 [m
[31m-		0x433d56,[m
[31m-		0x3b2854,[m
[31m-[m
[31m-		0x4d243c,[m
[31m-		0x384d47,[m
[31m-		0x323841,[m
[31m-		0x6c9799,[m
[31m-[m
[31m-		0xa1ab8c,[m
[31m-		0x3f4b5b,[m
[31m-[m
[31m-		// On[m
[31m-		0x000000,[m
[31m-[m
[31m-		0xa19856,[m
[31m-		0x63b1ff,[m
[31m-		0xff5e5e,[m
[31m-		0x66788e,[m
[31m-[m
[31m-		0x92ff63,[m
[31m-		0x63f2ff,[m
[31m-		0xffa200,[m
[31m-[m
[31m-		0xff628a,[m
[31m-		0x30d9ff,[m
[31m-		0xffc663,[m
[31m-[m
[31m-		0xae74ff,[m
[31m-		0xa600ff,[m
[31m-[m
[31m-		0xff0041,[m
[31m-		0x63ff9f,[m
[31m-		0xffffff,[m
[31m-		0xb2fbfe,[m
[31m-[m
[31m-		0xa1ab8c,[m
[31m-		0x3f4b5b[m
[31m-	};[m
[31m-[m
[31m-	const char* inkNames[] = {[m
[31m-		"None",[m
[31m-[m
[31m-		"Trace (Off)",[m
[31m-		"Read (Off)",[m
[31m-		"Write (Off)",[m
[31m-		"Cross",[m
[31m-[m
[31m-		"Buffer (Off)",[m
[31m-		"OR (Off)",[m
[31m-		"NAND (Off)",[m
[31m-[m
[31m-		"NOT (Off)",[m
[31m-		"NOR (Off)",[m
[31m-		"AND (Off)",[m
[31m-[m
[31m-		"XOR (Off)",[m
[31m-		"NXOR (Off)",[m
[31m-[m
[31m-		"Clock (Off)",[m
[31m-		"Latch (Off)",[m
[31m-		"LED (Off)",[m
[31m-		"Bundle (Off)",[m
[31m-[m
[31m-		"Filler",[m
[31m-		"Annotation",[m
[31m-[m
[31m-		"UNDEFINED",[m
[31m-[m
[31m-		"Trace (On)",[m
[31m-		"Read (On)",[m
[31m-		"Write (On)",[m
[31m-		"UNDEFINED",[m
[31m-[m
[31m-		"Buffer (On)",[m
[31m-		"OR (On)",[m
[31m-		"NAND (On)",[m
[31m-[m
[31m-		"NOT (On)",[m
[31m-		"NOR (On)",[m
[31m-		"AND (On)",[m
[31m-[m
[31m-		"XOR (On)",[m
[31m-		"NXOR (On)",[m
[31m-[m
[31m-		"Clock (On)",[m
[31m-		"Latch (On)",[m
[31m-		"LED (On)",[m
[31m-		"Bundle (On)",[m
[31m-[m
[31m-		"UNDEFINED",[m
[31m-		"UNDEFINED"[m
[31m-	};[m
[31m-[m
[31m-	const char* getInkString(Ink ink) {[m
[31m-		int i = (int)ink;[m
[31m-		if (i >= 128) i += (int)Ink::numTypes - 128;[m
[31m-		if (i < 0 || i > 2 * (int)Ink::numTypes) return "None";[m
[31m-		return inkNames[i];[m
[31m-	}[m
[31m-[m
[31m-	inline int col2int(int col) {[m
[31m-		int r = col & 0xff0000;[m
[31m-		int g = col & 0xff00;[m
[31m-		int b = col & 0xff;[m
[31m-		return (r >> 16) | g | (b << 16);[m
[31m-	}[m
[31m-[m
[31m-	int traceColors[] = {[m
[31m-			0x2a3541,[m
[31m-			0x9fa8ae,[m
[31m-			0xa1555e,[m
[31m-			0xa16c56,[m
[31m-			0xa18556,[m
[31m-			0xa19856,[m
[31m-			0x99a156,[m
[31m-			0x88a156,[m
[31m-			0x6ca156,[m
[31m-			0x56a18d,[m
[31m-			0x5693a1,[m
[31m-			0x567ba1,[m
[31m-			0x5662a1,[m
[31m-			0x6656a1,[m
[31m-			0x8756a1,[m
[31m-			0xa15597[m
[31m-	};[m
[31m-[m
[31m-	InkPixel color2ink(int col) {[m
[31m-		col = col2int(col);[m
[31m-		InkPixel pix = {};[m
[31m-		for (int i = 0; i < 16; i++)[m
[31m-			if (col == traceColors[i]) {[m
[31m-				pix.meta = (int16_t)i;[m
[31m-				pix.ink = (int16_t)Ink::Trace;[m
[31m-				break;[m
[31m-			}[m
[31m-[m
[31m-		switch (col) {[m
[31m-		case 0x3a4551:[m
[31m-			pix.ink = (int16_t)Ink::Annotation;[m
[31m-			break;[m
[31m-[m
[31m-		case 0x8caba1:[m
[31m-			pix.ink = (int16_t)Ink::Filler;[m
[31m-			break;[m
[31m-[m
[31m-		case 0x535572: // TUNNEL[m
[31m-			pix.ink = (int16_t)Ink::Tunnel;[m
[31m-			break;[m
[31m-[m
[31m-		case 0x646a57: // MESH[m
[31m-			pix.ink = (int16_t)Ink::Mesh;[m
[31m-			break;[m
[31m-[m
[31m-		// Bus components are treated as wire bundles (connectivity)[m
[31m-		case 0x7a2f24: // BUS_0 (122,47,36)[m
[31m-		case 0x3e7a24: // BUS_1 (62,122,36)[m
[31m-		case 0x24417a: // BUS_2 (36,65,122)[m
[31m-		case 0x25627a: // BUS_3 (37,98,122)[m
[31m-		case 0x7a2d66: // BUS_4 (122,45,102)[m
[31m-		case 0x7a7024: // BUS_5 (122,112,36)[m
[31m-			pix.ink = (int16_t)Ink::BundleOff;[m
[31m-			break;[m
[31m-[m
[31m-		default:[m
[31m-			for (int i = 0; i < 2 * (int)Ink::numTypes; i++)[m
[31m-				if (colorPallet[i] == col) {[m
[31m-					if (i >= (int)Ink::numTypes)[m
[31m-						i += 128 - (int)Ink::numTypes;[m
[31m-					pix.ink = (int16_t)i;[m
[31m-					break;[m
[31m-				}[m
[31m-		}[m
[31m-[m
[31m-		return pix;[m
[31m-	}[m
[31m-[m
[31m-	std::string split(std::string data, const char* t, int& start) {[m
[31m-		int tlen = strlen(t);[m
[31m-		int begin = start;[m
[31m-		int end = data.find(t, start + 1);[m
[31m-[m
[31m-		if (end < 0) {[m
[31m-			throw std::runtime_error(std::string("Missing expected token in .vcb file: [") + t + "]");[m
[31m-		}[m
[31m-[m
[31m-		start = end + tlen;[m
[31m-		return data.substr(begin, end - begin);[m
[31m-	}[m
[31m-[m
[31m-	bool processData(std::vector<unsigned char> logicData, int headerSize, int& width, int& height, unsigned char*& originalImage, unsigned long long& imSize) {[m
[31m-		int* header = (int*)&logicData[logicData.size() - headerSize];[m
[31m-[m
[31m-		const int imgDSize = header[5];[m
[31m-		width = header[3];[m
[31m-		height = header[1];[m
[31m-[m
[31m-		if (imgDSize != width * height * 4) {[m
[31m-			fprintf(stderr, "Error: Header width x height does not match header length\n");[m
[31m-			return 0;[m
[31m-		}[m
[31m-[m
[31m-		unsigned char* cc = &logicData[0];[m
[31m-		size_t ccSize = logicData.size() - headerSize;[m
[31m-[m
[31m-		imSize = ZSTD_getFrameContentSize(cc, ccSize);[m
[31m-[m
[31m-		if (imSize == ZSTD_CONTENTSIZE_ERROR) {[m
[31m-			fprintf(stderr, "Error: Data not compressed by zstd!\n");[m
[31m-			return 0;[m
[31m-		}[m
[31m-		else if (imSize == ZSTD_CONTENTSIZE_UNKNOWN) {[m
[31m-			fprintf(stderr, "Error: Original size unknown!\n");[m
[31m-			return 0;[m
[31m-		}[m
[31m-		else if (imSize != imgDSize) {[m
[31m-			fprintf(stderr, "Error: Decompressed image data size does not match header\n");[m
[31m-			return 0;[m
[31m-		}[m
[31m-		else {[m
[31m-			originalImage = new unsigned char[imSize];[m
[31m-			size_t const dSize = ZSTD_decompress(originalImage, imSize, cc, ccSize);[m
[31m-			return 1;[m
[31m-		}[m
[31m-	}[m
[31m-[m
[31m-	bool Project::processLogicData(std::vector<unsigned char> logicData, int headerSize) {[m
[31m-		unsigned long long imSize;[m
[31m-		if (processData(logicData, headerSize, width, height, originalImage, imSize)) {[m
[31m-			image = new InkPixel[imSize];[m
[32m+[m[32m        0x433d56,[m
[32m+[m[32m        0x3b2854,[m
[32m+[m
[32m+[m[32m        0x4d243c,[m
[32m+[m[32m        0x384d47,[m
[32m+[m[32m        0x323841,[m
[32m+[m[32m        0x6c9799,[m
[32m+[m
[32m+[m[32m        0xa1ab8c,[m
[32m+[m[32m        0x3f4b5b,[m
[32m+[m
[32m+[m[32m        // On[m
[32m+[m[32m        0x000000,[m
[32m+[m
[32m+[m[32m        0xa19856,[m
[32m+[m[32m        0x63b1ff,[m
[32m+[m[32m        0xff5e5e,[m
[32m+[m[32m        0x66788e,[m
[32m+[m
[32m+[m[32m        0x92ff63,[m
[32m+[m[32m        0x63f2ff,[m
[32m+[m[32m        0xffa200,[m
[32m+[m
[32m+[m[32m        0xff628a,[m
[32m+[m[32m        0x30d9ff,[m
[32m+[m[32m        0xffc663,[m
[32m+[m
[32m+[m[32m        0xae74ff,[m
[32m+[m[32m        0xa600ff,[m
[32m+[m
[32m+[m[32m        0xff0041,[m
[32m+[m[32m        0x63ff9f,[m
[32m+[m[32m        0xffffff,[m
[32m+[m[32m        0xb2fbfe,[m
[32m+[m
[32m+[m[32m        0xa1ab8c,[m
[32m+[m[32m        0x3f4b5b[m
[32m+[m[32m    };[m
[32m+[m
[32m+[m[32m    const char* inkNames[] = {[m
[32m+[m[32m        "None",[m
[32m+[m
[32m+[m[32m        "Trace (Off)",[m
[32m+[m[32m        "Read (Off)",[m
[32m+[m[32m        "Write (Off)",[m
[32m+[m[32m        "Cross",[m
[32m+[m
[32m+[m[32m        "Buffer (Off)",[m
[32m+[m[32m        "OR (Off)",[m
[32m+[m[32m        "NAND (Off)",[m
[32m+[m
[32m+[m[32m        "NOT (Off)",[m
[32m+[m[32m        "NOR (Off)",[m
[32m+[m[32m        "AND (Off)",[m
[32m+[m
[32m+[m[32m        "XOR (Off)",[m
[32m+[m[32m        "NXOR (Off)",[m
[32m+[m
[32m+[m[32m        "Clock (Off)",[m
[32m+[m[32m        "Latch (Off)",[m
[32m+[m[32m        "LED (Off)",[m
[32m+[m[32m        "Bundle (Off)",[m
[32m+[m
[32m+[m[32m        "Filler",[m
[32m+[m[32m        "Annotation",[m
[32m+[m
[32m+[m[32m        "UNDEFINED",[m
[32m+[m
[32m+[m[32m        "Trace (On)",[m
[32m+[m[32m        "Read (On)",[m
[32m+[m[32m        "Write (On)",[m
[32m+[m[32m        "UNDEFINED",[m
[32m+[m
[32m+[m[32m        "Buffer (On)",[m
[32m+[m[32m        "OR (On)",[m
[32m+[m[32m        "NAND (On)",[m
[32m+[m
[32m+[m[32m        "NOT (On)",[m
[32m+[m[32m        "NOR (On)",[m
[32m+[m[32m        "AND (On)",[m
[32m+[m
[32m+[m[32m        "XOR (On)",[m
[32m+[m[32m        "NXOR (On)",[m
[32m+[m
[32m+[m[32m        "Clock (On)",[m
[32m+[m[32m        "Latch (On)",[m
[32m+[m[32m        "LED (On)",[m
[32m+[m[32m        "Bundle (On)",[m
[32m+[m
[32m+[m[32m        "UNDEFINED",[m
[32m+[m[32m        "UNDEFINED"[m
[32m+[m[32m    };[m
[32m+[m
[32m+[m[32m    const char* getInkString(Ink ink) {[m
[32m+[m[32m        int i = (int)ink;[m
[32m+[m[32m        if (i >= 128) i += (int)Ink::numTypes - 128;[m
[32m+[m[32m        if (i < 0 || i > 2 * (int)Ink::numTypes) return "None";[m
[32m+[m[32m        return inkNames[i];[m
[32m+[m[32m    }[m
[32m+[m
[32m+[m[32m    inline int col2int(int col) {[m
[32m+[m[32m        int r = col & 0xff0000;[m
[32m+[m[32m        int g = col & 0xff00;[m
[32m+[m[32m        int b = col & 0xff;[m
[32m+[m[32m        return (r >> 16) | g | (b << 16);[m
[32m+[m[32m    }[m
[32m+[m
[32m+[m[32m    int traceColors[] = {[m
[32m+[m[32m            0x2a3541,[m
[32m+[m[32m            0x9fa8ae,[m
[32m+[m[32m            0xa1555e,[m
[32m+[m[32m            0xa16c56,[m
[32m+[m[32m            0xa18556,[m
[32m+[m[32m            0xa19856,[m
[32m+[m[32m            0x99a156,[m
[32m+[m[32m            0x88a156,[m
[32m+[m[32m            0x6ca156,[m
[32m+[m[32m            0x56a18d,[m
[32m+[m[32m            0x5693a1,[m
[32m+[m[32m            0x567ba1,[m
[32m+[m[32m            0x5662a1,[m
[32m+[m[32m            0x6656a1,[m
[32m+[m[32m            0x8756a1,[m
[32m+[m[32m            0xa15597[m
[32m+[m[32m    };[m
[32m+[m
[32m+[m[32m    InkPixel color2ink(int col) {[m
[32m+[m[32m        col = col2int(col);[m
[32m+[m[32m        InkPixel pix = {};[m
[32m+[m[32m        for (int i = 0; i < 16; i++)[m
[32m+[m[32m            if (col == traceColors[i]) {[m
[32m+[m[32m                pix.meta = (int16_t)i;[m
[32m+[m[32m                pix.ink = (int16_t)Ink::Trace;[m
[32m+[m[32m                break;[m
[32m+[m[32m            }[m
[32m+[m
[32m+[m[32m        switch (col) {[m
[32m+[m[32m        case 0x3a4551:[m
[32m+[m[32m            pix.ink = (int16_t)Ink::Annotation;[m
[32m+[m[32m            break;[m
[32m+[m
[32m+[m[32m        case 0x8caba1:[m
[32m+[m[32m            pix.ink = (int16_t)Ink::Filler;[m
[32m+[m[32m            break;[m
[32m+[m
[32m+[m[32m        case 0x535572: // TUNNEL (83,85,114)[m
[32m+[m[32m            pix.ink = (int16_t)Ink::Tunnel;[m
[32m+[m[32m            break;[m
[32m+[m
[32m+[m[32m        case 0x646a57: // MESH (100,106,87)[m
[32m+[m[32m            pix.ink = (int16_t)Ink::Mesh;[m
[32m+[m[32m            break;[m
[32m+[m
[32m+[m[32m        // Bus components are treated as wire bundles (connectivity)[m
[32m+[m[32m        case 0x7a2f24: // BUS_0 (122,47,36)[m
[32m+[m[32m        case 0x3e7a24: // BUS_1 (62,122,36)[m
[32m+[m[32m        case 0x24417a: // BUS_2 (36,65,122)[m
[32m+[m[32m        case 0x25627a: // BUS_3 (37,98,122)[m
[32m+[m[32m        case 0x7a2d66: // BUS_4 (122,45,102)[m
[32m+[m[32m        case 0x7a7024: // BUS_5 (122,112,36)[m
[32m+[m[32m            pix.ink = (int16_t)Ink::BundleOff;[m
[32m+[m[32m            break;[m
[32m+[m
[32m+[m[32m        default:[m
[32m+[m[32m            for (int i = 0; i < 2 * (int)Ink::numTypes; i++)[m
[32m+[m[32m                if (colorPallet[i] == col) {[m
[32m+[m[32m                    if (i >= (int)Ink::numTypes)[m
[32m+[m[32m                        i += 128 - (int)Ink::numTypes;[m
[32m+[m[32m                    pix.ink = (int16_t)i;[m
[32m+[m[32m                    break;[m
[32m+[m[32m                }[m
[32m+[m[32m        }[m
[32m+[m
[32m+[m[32m        return pix;[m
[32m+[m[32m    }[m
[32m+[m
[32m+[m[32m    std::string split(std::string data, const char* t, int& start) {[m
[32m+[m[32m        int tlen = strlen(t);[m
[32m+[m[32m        int begin = start;[m
[32m+[m[32m        int end = data.find(t, start + 1);[m
[32m+[m
[32m+[m[32m        if (end < 0) {[m
[32m+[m[32m            throw std::runtime_error(std::string("Missing expected token in .vcb file: [") + t + "]");[m
[32m+[m[32m        }[m
[32m+[m
[32m+[m[32m        start = end + tlen;[m
[32m+[m[32m        return data.substr(begin, end - begin);[m
[32m+[m[32m    }[m
[32m+[m
[32m+[m[32m    bool processData(std::vector<unsigned char> logicData, int headerSize, int& width, int& height, unsigned char*& originalImage, unsigned long long& imSize) {[m
[32m+[m[32m        int* header = (int*)&logicData[logicData.size() - headerSize];[m
[32m+[m
[32m+[m[32m        const int imgDSize = header[5];[m
[32m+[m[32m        width = header[3];[m
[32m+[m[32m        height = header[1];[m
[32m+[m
[32m+[m[32m        if (imgDSize != width * height * 4) {[m
[32m+[m[32m            fprintf(stderr, "Error: Header width x height does not match header length\n");[m
[32m+[m[32m            return 0;[m
[32m+[m[32m        }[m
[32m+[m
[32m+[m[32m        unsigned char* cc = &logicData[0];[m
[32m+[m[32m        size_t ccSize = logicData.size() - headerSize;[m
[32m+[m
[32m+[m[32m        imSize = ZSTD_getFrameContentSize(cc, ccSize);[m
[32m+[m
[32m+[m[32m        if (imSize == ZSTD_CONTENTSIZE_ERROR) {[m
[32m+[m[32m            fprintf(stderr, "Error: Data not compressed by zstd!\n");[m
[32m+[m[32m            return 0;[m
[32m+[m[32m        }[m
[32m+[m[32m        else if (imSize == ZSTD_CONTENTSIZE_UNKNOWN) {[m
[32m+[m[32m            fprintf(stderr, "Error: Original size unknown!\n");[m
[32m+[m[32m            return 0;[m
[32m+[m[32m        }[m
[32m+[m[32m        else if (imSize != imgDSize) {[m
[32m+[m[32m            fprintf(stderr, "Error: Decompressed image data size does not match header\n");[m
[32m+[m[32m            return 0;[m
[32m+[m[32m        }[m
[32m+[m[32m        else {[m
[32m+[m[32m            originalImage = new unsigned char[imSize];[m
[32m+[m[32m            size_t const dSize = ZSTD_decompress(originalImage, imSize, cc, ccSize);[m
[32m+[m[32m            return 1;[m
[32m+[m[32m        }[m
[32m+[m[32m    }[m
[32m+[m
[32m+[m[32m    bool Project::processLogicData(std::vector<unsigned char> logicData, int headerSize) {[m
[32m+[m[32m        unsigned long long imSize;[m
[32m+[m[32m        if (processData(logicData, headerSize, width, height, originalImage, imSize)) {[m
[32m+[m[32m            image = new InkPixel[imSize];[m
 #pragma omp parallel for schedule(static, 8196)[m
[31m-			for (int i = 0; i < imSize / 4; i++)[m
[31m-				image[i] = color2ink(((int*)originalImage)[i]);[m
[31m-[m
[31m-			return 1;[m
[31m-		}[m
[31m-[m
[31m-		return 0;[m
[31m-	}[m
[31m-[m
[31m-	void Project::Project::processDecorationData(std::vector<unsigned char> decorationData, int*& decoData) {[m
[31m-		unsigned long long imSize;[m
[31m-		int width, height;[m
[31m-		if (processData(decorationData, 24, width, height, (unsigned char*&)decoData, imSize)) {[m
[31m-			for (int i = 0; i < imSize / 4; i++) {[m
[31m-				int color = decoData[i];[m
[31m-				decoData[i] = col2int(color);[m
[31m-			}[m
[31m-		}[m
[31m-	}[m
[31m-[m
[31m-	bool Project::readFromVCB(std::string filePath) {[m
[31m-		std::ifstream stream(filePath, std::ios::binary);[m
[31m-		if (!stream.is_open()) {[m
[31m-			fprintf(stderr, "Error: Cannot open file for reading: \"%s\"\n", filePath.c_str());[m
[31m-			return false;[m
[31m-		}[m
[31m-[m
[31m-		std::stringstream ss;[m
[31m-		ss << stream.rdbuf();[m
[31m-		std::string content = ss.str();[m
[31m-		stream.close();[m
[31m-[m
[31m-		if (content.empty()) {[m
[31m-			fprintf(stderr, "Error: File \"%s\" is empty\n", filePath.c_str());[m
[31m-			return false;[m
[31m-		}[m
[31m-[m
[31m-		bool isGodot = (content.find("PoolByteArray") != std::string::npos ||[m
[31m-		                content.find("[gd_resource") != std::string::npos);[m
[31m-[m
[31m-		try {[m
[31m-			if (isGodot) {[m
[31m-				return readFromGodotVCB(content);[m
[31m-			} else {[m
[31m-				return readFromJsonVCB(content);[m
[31m-			}[m
[31m-		} catch (const std::exception& e) {[m
[31m-			fprintf(stderr, "Error parsing VCB file \"%s\": %s\n", filePath.c_str(), e.what());[m
[31m-			return false;[m
[31m-		}[m
[31m-	}[m
[31m-[m
[31m-	bool Project::readFromJsonVCB(const std::string& json) {[m
[31m-		// 1. assembly[m
[31m-		size_t pos = json.find("\"assembly\": \"");[m
[31m-		if (pos != std::string::npos) {[m
[31m-			pos += 13;[m
[31m-			size_t end = json.find("\",", pos);[m
[31m-			if (end != std::string::npos) {[m
[31m-				assembly = json.substr(pos, end - pos);[m
[31m-			}[m
[31m-		}[m
[31m-[m
[31m-		// 2. is_vmem_enabled[m
[31m-		bool vmemFlag = false;[m
[31m-		pos = json.find("\"is_vmem_enabled\":");[m
[31m-		if (pos != std::string::npos) {[m
[31m-			size_t valStart = json.find_first_not_of(" \t\r\n", pos + 18);[m
[31m-			if (valStart != std::string::npos && json.substr(valStart, 4) == "true") {[m
[31m-				vmemFlag = true;[m
[31m-			}[m
[31m-		}[m
[31m-[m
[31m-		// 3. layers (base64 zstd streams)[m
[31m-		pos = json.find("\"layers\":");[m
[31m-		if (pos == std::string::npos) {[m
[31m-			throw std::runtime_error("Missing \"layers\" key in JSON .vcb file");[m
[31m-		}[m
[31m-		pos = json.find("[", pos);[m
[31m-		std::vector<std::string> b64Layers;[m
[31m-		while (pos != std::string::npos) {[m
[31m-			size_t strStart = json.find("\"", pos + 1);[m
[31m-			if (strStart == std::string::npos) break;[m
[31m-			size_t nextBracket = json.find("]", pos + 1);[m
[31m-			if (nextBracket != std::string::npos && strStart > nextBracket) break;[m
[31m-			size_t strEnd = json.find("\"", strStart + 1);[m
[31m-			if (strEnd == std::string::npos) break;[m
[31m-			b64Layers.push_back(json.substr(strStart + 1, strEnd - strStart - 1));[m
[31m-			pos = strEnd;[m
[31m-		}[m
[31m-[m
[31m-		if (b64Layers.empty()) {[m
[31m-			throw std::runtime_error("No layers found in JSON .vcb file");[m
[31m-		}[m
[31m-[m
[31m-		// Layer 0 is logic data[m
[31m-		std::vector<unsigned char> logicData = b64decode(b64Layers[0]);[m
[31m-[m
[31m-		// Layers 1..3 are decoration data[m
[31m-		for (size_t i = 1; i < b64Layers.size() && i <= 3; i++) {[m
[31m-			std::vector<unsigned char> decoData = b64decode(b64Layers[i]);[m
[31m-			processDecorationData(decoData, decoration[i - 1]);[m
[31m-		}[m
[31m-[m
[31m-		// 4. led_palette[m
[31m-		pos = json.find("\"led_palette\":");[m
[31m-		if (pos != std::string::npos) {[m
[31m-			size_t bStart = json.find("[", pos);[m
[31m-			size_t bEnd = json.find("]", bStart);[m
[31m-			if (bStart != std::string::npos && bEnd != std::string::npos) {[m
[31m-				std::string palStr = json.substr(bStart + 1, bEnd - bStart - 1);[m
[31m-				std::stringstream s(palStr);[m
[31m-				std::string item;[m
[31m-				int idx = 0;[m
[31m-				while (std::getline(s, item, ',') && idx < 16) {[m
[31m-					item.erase(std::remove(item.begin(), item.end(), '\"'), item.end());[m
[31m-					item.erase(std::remove_if(item.begin(), item.end(), ::isspace), item.end());[m
[31m-					if (!item.empty()) {[m
[31m-						ledPalette[idx++] = std::stoul(item, nullptr, 16);[m
[31m-					}[m
[31m-				}[m
[31m-			}[m
[31m-		}[m
[31m-[m
[31m-		// 5. vmem_settings[m
[31m-		pos = json.find("\"vmem_settings\":");[m
[31m-		int vmemArr[14] = {0};[m
[31m-		if (pos != std::string::npos) {[m
[31m-			size_t bStart = json.find("[", pos);[m
[31m-			size_t bEnd = json.find("]", bStart);[m
[31m-			if (bStart != std::string::npos && bEnd != std::string::npos) {[m
[31m-				std::string arrStr = json.substr(bStart + 1, bEnd - bStart - 1);[m
[31m-				std::stringstream s(arrStr);[m
[31m-				std::string item;[m
[31m-				for (int i = 0; i < 14 && std::getline(s, item, ','); i++) {[m
[31m-					vmemArr[i] = std::atoi(item.c_str());[m
[31m-				}[m
[31m-			}[m
[31m-		}[m
[31m-[m
[31m-		// Set the vmem settings[m
[31m-		vmAddr.numBits = std::max(0, std::min(vmemArr[0], 32));[m
[31m-		vmAddr.pos.x = vmemArr[1];[m
[31m-		vmAddr.pos.y = vmemArr[2];[m
[31m-		vmAddr.stride.x = -vmemArr[3];[m
[31m-		vmAddr.stride.y = vmemArr[4];[m
[31m-		vmAddr.size.x = vmemArr[5];[m
[31m-		vmAddr.size.y = vmemArr[6];[m
[31m-[m
[31m-		vmData.numBits = std::max(0, std::min(vmemArr[7], 32));[m
[31m-		vmData.pos.x = vmemArr[8];[m
[31m-		vmData.pos.y = vmemArr[9];[m
[31m-		vmData.stride.x = -vmemArr[10];[m
[31m-		vmData.stride.y = vmemArr[11];[m
[31m-		vmData.size.x = vmemArr[12];[m
[31m-		vmData.size.y = vmemArr[13];[m
[31m-[m
[31m-		vmWe.numBits = 0;[m
[31m-		vmWe.gids[0] = -1;[m
[31m-[m
[31m-		vmWe.numBits = 0;[m
[31m-		vmWe.gids[0] = -1;[m
[31m-[m
[31m-		if (vmemFlag) {[m
[31m-			vmemSize = 1ull << vmAddr.numBits;[m
[31m-			vmem = new int[vmemSize];[m
[31m-			memset(vmem, 0, 4 * vmemSize);[m
[31m-		}[m
[31m-		[m
[31m-		// Load initial virtual memory contents from "vmem_data" (base64 + zstd)[m
[32m+[m[32m            for (int i = 0; i < imSize / 4; i++)[m
[32m+[m[32m                image[i] = color2ink(((int*)originalImage)[i]);[m
[32m+[m
[32m+[m[32m            return 1;[m
[32m+[m[32m        }[m
[32m+[m
[32m+[m[32m        return 0;[m
[32m+[m[32m    }[m
[32m+[m
[32m+[m[32m    void Project::Project::processDecorationData(std::vector<unsigned char> decorationData, int*& decoData) {[m
[32m+[m[32m        unsigned long long imSize;[m
[32m+[m[32m        int width, height;[m
[32m+[m[32m        if (processData(decorationData, 24, width, height, (unsigned char*&)decoData, imSize)) {[m
[32m+[m[32m            for (int i = 0; i < imSize / 4; i++) {[m
[32m+[m[32m                int color = decoData[i];[m
[32m+[m[32m                decoData[i] = col2int(color);[m
[32m+[m[32m            }[m
[32m+[m[32m        }[m
[32m+[m[32m    }[m
[32m+[m
[32m+[m[32m    bool Project::readFromVCB(std::string filePath) {[m
[32m+[m[32m        std::ifstream stream(filePath, std::ios::binary);[m
[32m+[m[32m        if (!stream.is_open()) {[m
[32m+[m[32m            fprintf(stderr, "Error: Cannot open file for reading: \"%s\"\n", filePath.c_str());[m
[32m+[m[32m            return false;[m
[32m+[m[32m        }[m
[32m+[m
[32m+[m[32m        std::stringstream ss;[m
[32m+[m[32m        ss << stream.rdbuf();[m
[32m+[m[32m        std::string content = ss.str();[m
[32m+[m[32m        stream.close();[m
[32m+[m
[32m+[m[32m        if (content.empty()) {[m
[32m+[m[32m            fprintf(stderr, "Error: File \"%s\" is empty\n", filePath.c_str());[m
[32m+[m[32m            return false;[m
[32m+[m[32m        }[m
[32m+[m
[32m+[m[32m        bool isGodot = (content.find("PoolByteArray") != std::string::npos ||[m
[32m+[m[32m                        content.find("[gd_resource") != std::string::npos);[m
[32m+[m
[32m+[m[32m        try {[m
[32m+[m[32m            if (isGodot) {[m
[32m+[m[32m                return readFromGodotVCB(content);[m
[32m+[m[32m            } else {[m
[32m+[m[32m                return readFromJsonVCB(content);[m
[32m+[m[32m            }[m
[32m+[m[32m        } catch (const std::exception& e) {[m
[32m+[m[32m            fprintf(stderr, "Error parsing VCB file \"%s\": %s\n", filePath.c_str(), e.what());[m
[32m+[m[32m            return false;[m
[32m+[m[32m        }[m
[32m+[m[32m    }[m
[32m+[m
[32m+[m[32m    bool Project::readFromJsonVCB(const std::string& json) {[m
[32m+[m[32m        // 1. assembly[m
[32m+[m[32m        size_t pos = json.find("\"assembly\": \"");[m
[32m+[m[32m        if (pos != std::string::npos) {[m
[32m+[m[32m            pos += 13;[m
[32m+[m[32m            size_t end = json.find("\",", pos);[m
[32m+[m[32m            if (end != std::string::npos) {[m
[32m+[m[32m                assembly = json.substr(pos, end - pos);[m
[32m+[m[32m            }[m
[32m+[m[32m        }[m
[32m+[m
[32m+[m[32m        // 2. is_vmem_enabled[m
[32m+[m[32m        bool vmemFlag = false;[m
[32m+[m[32m        pos = json.find("\"is_vmem_enabled\":");[m
[32m+[m[32m        if (pos != std::string::npos) {[m
[32m+[m[32m            size_t valStart = json.find_first_not_of(" \t\r\n", pos + 18);[m
[32m+[m[32m            if (valStart != std::string::npos && json.substr(valStart, 4) == "true") {[m
[32m+[m[32m                vmemFlag = true;[m
[32m+[m[32m            }[m
[32m+[m[32m        }[m
[32m+[m
[32m+[m[32m        // 3. layers (base64 zstd streams)[m
[32m+[m[32m        pos = json.find("\"layers\":");[m
[32m+[m[32m        if (pos == std::string::npos) {[m
[32m+[m[32m            throw std::runtime_error("Missing \"layers\" key in JSON .vcb file");[m
[32m+[m[32m        }[m
[32m+[m[32m        pos = json.find("[", pos);[m
[32m+[m[32m        std::vector<std::string> b64Layers;[m
[32m+[m[32m        while (pos != std::string::npos) {[m
[32m+[m[32m            size_t strStart = json.find("\"", pos + 1);[m
[32m+[m[32m            if (strStart == std::string::npos) break;[m
[32m+[m[32m            size_t nextBracket = json.find("]", pos + 1);[m
[32m+[m[32m            if (nextBracket != std::string::npos && strStart > nextBracket) break;[m
[32m+[m[32m            size_t strEnd = json.find("\"", strStart + 1);[m
[32m+[m[32m            if (strEnd == std::string::npos) break;[m
[32m+[m[32m            b64Layers.push_back(json.substr(strStart + 1, strEnd - strStart - 1));[m
[32m+[m[32m            pos = strEnd;[m
[32m+[m[32m        }[m
[32m+[m
[32m+[m[32m        if (b64Layers.empty()) {[m
[32m+[m[32m            throw std::runtime_error("No layers found in JSON .vcb file");[m
[32m+[m[32m        }[m
[32m+[m
[32m+[m[32m        // Layer 0 is logic data[m
[32m+[m[32m        std::vector<unsigned char> logicData = b64decode(b64Layers[0]);[m
[32m+[m
[32m+[m[32m        // Layers 1..3 are decoration data[m
[32m+[m[32m        for (size_t i = 1; i < b64Layers.size() && i <= 3; i++) {[m
[32m+[m[32m            std::vector<unsigned char> decoData = b64decode(b64Layers[i]);[m
[32m+[m[32m            processDecorationData(decoData, decoration[i - 1]);[m
[32m+[m[32m        }[m
[32m+[m
[32m+[m[32m        // 4. led_palette[m
[32m+[m[32m        pos = json.find("\"led_palette\":");[m
[32m+[m[32m        if (pos != std::string::npos) {[m
[32m+[m[32m            size_t bStart = json.find("[", pos);[m
[32m+[m[32m            size_t bEnd = json.find("]", bStart);[m
[32m+[m[32m            if (bStart != std::string::npos && bEnd != std::string::npos) {[m
[32m+[m[32m                std::string palStr = json.substr(bStart + 1, bEnd - bStart - 1);[m
[32m+[m[32m                std::stringstream s(palStr);[m
[32m+[m[32m                std::string item;[m
[32m+[m[32m                int idx = 0;[m
[32m+[m[32m                while (std::getline(s, item, ',') && idx < 16) {[m
[32m+[m[32m                    item.erase(std::remove(item.begin(), item.end(), '\"'), item.end());[m
[32m+[m[32m                    item.erase(std::remove_if(item.begin(), item.end(), ::isspace), item.end());[m
[32m+[m[32m                    if (!item.empty()) {[m
[32m+[m[32m                        ledPalette[idx++] = std::stoul(item, nullptr, 16);[m
[32m+[m[32m                    }[m
[32m+[m[32m                }[m
[32m+[m[32m            }[m
[32m+[m[32m        }[m
[32m+[m
[32m+[m[32m        // 5. vmem_settings[m
[32m+[m[32m        pos = json.find("\"vmem_settings\":");[m
[32m+[m[32m        int vmemArr[14] = {0};[m
[32m+[m[32m        if (pos != std::string::npos) {[m
[32m+[m[32m            size_t bStart = json.find("[", pos);[m
[32m+[m[32m            size_t bEnd = json.find("]", bStart);[m
[32m+[m[32m            if (bStart != std::string::npos && bEnd != std::string::npos) {[m
[32m+[m[32m                std::string arrStr = json.substr(bStart + 1, bEnd - bStart - 1);[m
[32m+[m[32m                std::stringstream s(arrStr);[m
[32m+[m[32m                std::string item;[m
[32m+[m[32m                for (int i = 0; i < 14 && std::getline(s, item, ','); i++) {[m
[32m+[m[32m                    vmemArr[i] = std::atoi(item.c_str());[m
[32m+[m[32m                }[m
[32m+[m[32m            }[m
[32m+[m[32m        }[m
[32m+[m
[32m+[m[32m        // Set the vmem settings[m
[32m+[m[32m        vmAddr.numBits = std::max(0, std::min(vmemArr[0], 32));[m
[32m+[m[32m        vmAddr.pos.x = vmemArr[1];[m
[32m+[m[32m        vmAddr.pos.y = vmemArr[2];[m
[32m+[m[32m        vmAddr.stride.x = -vmemArr[3];[m
[32m+[m[32m        vmAddr.stride.y = vmemArr[4];[m
[32m+[m[32m        vmAddr.size.x = vmemArr[5];[m
[32m+[m[32m        vmAddr.size.y = vmemArr[6];[m
[32m+[m
[32m+[m[32m        vmData.numBits = std::max(0, std::min(vmemArr[7], 32));[m
[32m+[m[32m        vmData.pos.x = vmemArr[8];[m
[32m+[m[32m        vmData.pos.y = vmemArr[9];[m
[32m+[m[32m        vmData.stride.x = -vmemArr[10];[m
[32m+[m[32m        vmData.stride.y = vmemArr[11];[m
[32m+[m[32m        vmData.size.x = vmemArr[12];[m
[32m+[m[32m        vmData.size.y = vmemArr[13];[m
[32m+[m
[32m+[m[32m        vmWe.numBits = 0;[m
[32m+[m[32m        vmWe.gids[0] = -1;[m
[32m+[m
[32m+[m[32m        if (vmemFlag) {[m
[32m+[m[32m            vmemSize = 1ull << vmAddr.numBits;[m
[32m+[m[32m            vmem = new int[vmemSize];[m
[32m+[m[32m            memset(vmem, 0, 4 * vmemSize);[m
[32m+[m[32m        }[m
[32m+[m[41m        [m
[32m+[m[32m        // Load initial virtual memory contents from "vmem_data" (base64 + zstd)[m
         if (vmemFlag) {[m
             size_t vdPos = json.find("\"vmem_data\":");[m
             if (vdPos != std::string::npos) {[m
[36m@@ -452,7 +449,6 @@[m [mnamespace openVCB {[m
                                 size_t dSize = 0;[m
                                 bool success = false;[m
                                 [m
[31m-                                // Try decompressing while trimming trailing bytes to bypass base64 padding mismatch[m
                                 for (size_t trim = 0; trim <= 64 && comp.size() > trim; trim++) {[m
                                     size_t srcSize = comp.size() - trim;[m
                                     size_t res = ZSTD_decompress(buf.data(), decompSize, comp.data(), srcSize);[m
[36m@@ -466,7 +462,6 @@[m [mnamespace openVCB {[m
                                 if (success) {[m
                                     size_t numInts = dSize / 4;[m
                                     size_t count = std::min(numInts, (size_t)vmemSize);[m
[31m-                                    fprintf(stderr, "[DEBUG] Successfully decompressed vmem_data! Loaded %zu ints into vmem.\n", count);[m
                                     for (size_t i = 0; i < count; i++) {[m
                                         unsigned int val = ((unsigned int)buf[i*4] << 24) |[m
                                                            ((unsigned int)buf[i*4+1] << 16) |[m
[36m@@ -474,8 +469,6 @@[m [mnamespace openVCB {[m
                                                            ((unsigned int)buf[i*4+3]);[m
                                         vmem[i] = (int)val;[m
                                     }[m
[31m-                                } else {[m
[31m-                                    fprintf(stderr, "[DEBUG] Zstd decompress failed for vmem_data completely.\n");[m
                                 }[m
                             }[m
                         }[m
[36m@@ -484,182 +477,174 @@[m [mnamespace openVCB {[m
             }[m
         }[m
 [m
[31m-		if (!Project::processLogicData(logicData, 24)) {[m
[31m-			return false;[m
[31m-		}[m
[31m-[m
[31m-		if (vmemFlag) {[m
[31m-			for (int i = 0; i < vmAddr.numBits; i++) {[m
[31m-				auto start = vmAddr.pos + i * vmAddr.stride;[m
[31m-				auto end = start + vmAddr.size;[m
[31m-				for (auto p = start; p.x < end.x; p.x++) {[m
[31m-					for (p.y = start.y; p.y < end.y; p.y++) {[m
[31m-						if (p.x < 0 || p.x >= width || p.y < 0 || p.y >= height)[m
[31m-							continue;[m
[31m-						image[p.x + p.y * width].ink = (int16_t)Ink::LatchOff;[m
[31m-					}[m
[31m-				}[m
[31m-			}[m
[31m-[m
[31m-			for (int i = 0; i < vmData.numBits; i++) {[m
[31m-				auto start = vmData.pos + i * vmData.stride;[m
[31m-				auto end = start + vmData.size;[m
[31m-				for (auto p = start; p.x < end.x; p.x++) {[m
[31m-					for (p.y = start.y; p.y < end.y; p.y++) {[m
[31m-						if (p.x < 0 || p.x >= width || p.y < 0 || p.y >= height)[m
[31m-							continue;[m
[31m-						image[p.x + p.y * width].ink = (int16_t)Ink::LatchOff;[m
[31m-					}[m
[31m-				}[m
[31m-			}[m
[31m-		}[m
[31m-[m
[31m-		return true;[m
[31m-	}[m
[31m-[m
[31m-	bool Project::readFromGodotVCB(const std::string& godotObj) {[m
[31m-		std::string obj = godotObj;[m
[31m-[m
[31m-		// split out assembly[m
[31m-		int pos = 0;[m
[31m-		split(obj, "\"assembly\": \"", pos);[m
[31m-		assembly = split(obj, "\",", pos);[m
[31m-[m
[31m-		// if VMem is enabled[m
[31m-		split(obj, "\"is_vmem_enabled\": ", pos);[m
[31m-		auto vmemFlag = split(obj, ",", pos) == "true";[m
[31m-[m
[31m-		std::vector<unsigned char> logicData;[m
[31m-		split(obj, "PoolByteArray(", pos);[m
[31m-[m
[31m-		{[m
[31m-			auto dat = split(obj, " ), PoolByteArray( ", pos);[m
[31m-			std::stringstream s(dat);[m
[31m-			std::string val;[m
[31m-			while (std::getline(s, val, ','))[m
[31m-				logicData.push_back(atoi(val.c_str() + 1));[m
[31m-		}[m
[31m-[m
[31m-		std::vector<unsigned char> decorationData[3];[m
[31m-		{[m
[31m-			pos--;[m
[31m-			auto dat = split(obj, " ), PoolByteArray( ", pos);[m
[31m-			std::stringstream s(dat);[m
[31m-			std::string val;[m
[31m-			while (std::getline(s, val, ','))[m
[31m-				decorationData[0].push_back(atoi(val.c_str() + 1));[m
[31m-		}[m
[31m-[m
[31m-		{[m
[31m-			pos--;[m
[31m-			auto dat = split(obj, " ), PoolByteArray( ", pos);[m
[31m-			std::stringstream s(dat);[m
[31m-			std::string val;[m
[31m-			while (std::getline(s, val, ','))[m
[31m-				decorationData[1].push_back(atoi(val.c_str() + 1));[m
[31m-		}[m
[31m-[m
[31m-		{[m
[31m-			pos--;[m
[31m-			auto dat = split(obj, " ) ]", pos);[m
[31m-			std::stringstream s(dat);[m
[31m-			std::string val;[m
[31m-			while (std::getline(s, val, ','))[m
[31m-				decorationData[2].push_back(atoi(val.c_str() + 1));[m
[31m-		}[m
[31m-[m
[31m-		//led palette[m
[31m-		std::vector<int> vecPalette;[m
[31m-		split(obj, "\"led_palette\": [ ", pos);[m
[31m-[m
[31m-		auto dat = split(obj, " ]", pos);[m
[31m-		std::stringstream s(dat);[m
[31m-		std::string val;[m
[31m-		while (std::getline(s, val, ',')) {[m
[31m-			val.erase(remove(val.begin(), val.end(), '\"'), val.end());[m
[31m-			vecPalette.push_back(std::stoul(val, nullptr, 16));[m
[31m-		}[m
[31m-[m
[31m-		for (int i = 0; i < std::min((int)vecPalette.size(), 16) ; i++) {[m
[31m-			ledPalette[i] = vecPalette[i];[m
[31m-		}[m
[31m-[m
[31m-		split(obj, "\"vmem_settings\": [ ", pos);[m
[31m-[m
[31m-		// Get VMem settings[m
[31m-		int vmemArr[14];[m
[31m-		{[m
[31m-			auto dat = split(obj, " ]", pos);[m
[31m-			std::stringstream s(dat);[m
[31m-			std::string val;[m
[31m-			for (size_t i = 0; i < 14; i++) {[m
[31m-				std::getline(s, val, ',');[m
[31m-				vmemArr[i] = atoi(val.c_str());[m
[31m-			}[m
[31m-		}[m
[31m-[m
[31m-		// Set the vmem settings[m
[31m-		vmAddr.numBits = std::max(0, std::min(vmemArr[0], 32));[m
[31m-		vmAddr.pos.x = vmemArr[1];[m
[31m-		vmAddr.pos.y = vmemArr[2];[m
[31m-		vmAddr.stride.x = -vmemArr[3];[m
[31m-		vmAddr.stride.y = vmemArr[4];[m
[31m-		vmAddr.size.x = vmemArr[5];[m
[31m-		vmAddr.size.y = vmemArr[6];[m
[31m-[m
[31m-		vmData.numBits = std::max(0, std::min(vmemArr[7], 32));[m
[31m-		vmData.pos.x = vmemArr[8];[m
[31m-		vmData.pos.y = vmemArr[9];[m
[31m-		vmData.stride.x = -vmemArr[10];[m
[31m-		vmData.stride.y = vmemArr[11];[m
[31m-		vmData.size.x = vmemArr[12];[m
[31m-		vmData.size.y = vmemArr[13];[m
[31m-[m
[31m-		vmWe.numBits = 0;[m
[31m-		vmWe.gids[0] = -1;[m
[31m-[m
[31m-		vmWe.numBits = 0;[m
[31m-		vmWe.gids[0] = -1;[m
[31m-[m
[31m-		if (vmemFlag) {[m
[31m-			vmemSize = 1ull << vmAddr.numBits;[m
[31m-			vmem = new int[vmemSize];[m
[31m-			memset(vmem, 0, 4 * vmemSize);[m
[31m-		}[m
[31m-[m
[31m-		if (!Project::processLogicData(logicData, 24)) {[m
[31m-			return false;[m
[31m-		}[m
[31m-[m
[31m-		if (vmemFlag) {[m
[31m-			for (int i = 0; i < vmAddr.numBits; i++) {[m
[31m-				auto start = vmAddr.pos + i * vmAddr.stride;[m
[31m-				auto end = start + vmAddr.size;[m
[31m-				for (auto p = start; p.x < end.x; p.x++) {[m
[31m-					for (p.y = start.y; p.y < end.y; p.y++) {[m
[31m-						if (p.x < 0 || p.x >= width || p.y < 0 || p.y >= height)[m
[31m-							continue;[m
[31m-						image[p.x + p.y * width].ink = (int16_t)Ink::LatchOff;[m
[31m-					}[m
[31m-				}[m
[31m-			}[m
[31m-[m
[31m-			for (int i = 0; i < vmData.numBits; i++) {[m
[31m-				auto start = vmData.pos + i * vmData.stride;[m
[31m-				auto end = start + vmData.size;[m
[31m-				for (auto p = start; p.x < end.x; p.x++) {[m
[31m-					for (p.y = start.y; p.y < end.y; p.y++) {[m
[31m-						if (p.x < 0 || p.x >= width || p.y < 0 || p.y >= height)[m
[31m-							continue;[m
[31m-						image[p.x + p.y * width].ink = (int16_t)Ink::LatchOff;[m
[31m-					}[m
[31m-				}[m
[31m-			}[m
[31m-		}[m
[31m-[m
[31m-		Project::processDecorationData(decorationData[0], decoration[0]);[m
[31m-		Project::processDecorationData(decorationData[1], decoration[1]);[m
[31m-		Project::processDecorationData(decorationData[2], decoration[2]);[m
[31m-		return true;[m
[31m-	}[m
[32m+[m[32m        if (!Project::processLogicData(logicData, 24)) {[m
[32m+[m[32m            return false;[m
[32m+[m[32m        }[m
[32m+[m
[32m+[m[32m        if (vmemFlag) {[m
[32m+[m[32m            for (int i = 0; i < vmAddr.numBits; i++) {[m
[32m+[m[32m                auto start = vmAddr.pos + i * vmAddr.stride;[m
[32m+[m[32m                auto end = start + vmAddr.size;[m
[32m+[m[32m                for (auto p = start; p.x < end.x; p.x++) {[m
[32m+[m[32m                    for (p.y = start.y; p.y < end.y; p.y++) {[m
[32m+[m[32m                        if (p.x < 0 || p.x >= width || p.y < 0 || p.y >= height)[m
[32m+[m[32m                            continue;[m
[32m+[m[32m                        image[p.x + p.y * width].ink = (int16_t)Ink::LatchOff;[m
[32m+[m[32m                    }[m
[32m+[m[32m                }[m
[32m+[m[32m            }[m
[32m+[m
[32m+[m[32m            for (int i = 0; i < vmData.numBits; i++) {[m
[32m+[m[32m                auto start = vmData.pos + i * vmData.stride;[m
[32m+[m[32m                auto end = start + vmData.size;[m
[32m+[m[32m                for (auto p = start; p.x < end.x; p.x++) {[m
[32m+[m[32m                    for (p.y = start.y; p.y < end.y; p.y++) {[m
[32m+[m[32m                        if (p.x < 0 || p.x >= width || p.y < 0 || p.y >= height)[m
[32m+[m[32m                            continue;[m
[32m+[m[32m                        image[p.x + p.y * width].ink = (int16_t)Ink::LatchOff;[m
[32m+[m[32m                    }[m
[32m+[m[32m                }[m
[32m+[m[32m            }[m
[32m+[m[32m        }[m
[32m+[m
[32m+[m[32m        return true;[m
[32m+[m[32m    }[m
[32m+[m
[32m+[m[32m    bool Project::readFromGodotVCB(const std::string& godotObj) {[m
[32m+[m[32m        std::string obj = godotObj;[m
[32m+[m
[32m+[m[32m        int pos = 0;[m
[32m+[m[32m        split(obj, "\"assembly\": \"", pos);[m
[32m+[m[32m        assembly = split(obj, "\",", pos);[m
[32m+[m
[32m+[m[32m        split(obj, "\"is_vmem_enabled\": ", pos);[m
[32m+[m[32m        auto vmemFlag = split(obj, ",", pos) == "true";[m
[32m+[m
[32m+[m[32m        std::vector<unsigned char> logicData;[m
[32m+[m[32m        split(obj, "PoolByteArray(", pos);[m
[32m+[m
[32m+[m[32m        {[m
[32m+[m[32m            auto dat = split(obj, " ), PoolByteArray( ", pos);[m
[32m+[m[32m            std::stringstream s(dat);[m
[32m+[m[32m            std::string val;[m
[32m+[m[32m            while (std::getline(s, val, ','))[m
[32m+[m[32m                logicData.push_back(atoi(val.c_str() + 1));[m
[32m+[m[32m        }[m
[32m+[m
[32m+[m[32m        std::vector<unsigned char> decorationData[3];[m
[32m+[m[32m        {[m
[32m+[m[32m            pos--;[m
[32m+[m[32m            auto dat = split(obj, " ), PoolByteArray( ", pos);[m
[32m+[m[32m            std::stringstream s(dat);[m
[32m+[m[32m            std::string val;[m
[32m+[m[32m            while (std::getline(s, val, ','))[m
[32m+[m[32m                decorationData[0].push_back(atoi(val.c_str() + 1));[m
[32m+[m[32m        }[m
[32m+[m
[32m+[m[32m        {[m
[32m+[m[32m            pos--;[m
[32m+[m[32m            auto dat = split(obj, " ), PoolByteArray( ", pos);[m
[32m+[m[32m            std::stringstream s(dat);[m
[32m+[m[32m            std::string val;[m
[32m+[m[32m            while (std::getline(s, val, ','))[m
[32m+[m[32m                decorationData[1].push_back(atoi(val.c_str() + 1));[m
[32m+[m[32m        }[m
[32m+[m
[32m+[m[32m        {[m
[32m+[m[32m            pos--;[m
[32m+[m[32m            auto dat = split(obj, " ) ]", pos);[m
[32m+[m[32m            std::stringstream s(dat);[m
[32m+[m[32m            std::string val;[m
[32m+[m[32m            while (std::getline(s, val, ','))[m
[32m+[m[32m                decorationData[2].push_back(atoi(val.c_str() + 1));[m
[32m+[m[32m        }[m
[32m+[m
[32m+[m[32m        std::vector<int> vecPalette;[m
[32m+[m[32m        split(obj, "\"led_palette\": [ ", pos);[m
[32m+[m
[32m+[m[32m        auto dat = split(obj, " ]", pos);[m
[32m+[m[32m        std::stringstream s(dat);[m
[32m+[m[32m        std::string val;[m
[32m+[m[32m        while (std::getline(s, val, ',')) {[m
[32m+[m[32m            val.erase(remove(val.begin(), val.end(), '\"'), val.end());[m
[32m+[m[32m            vecPalette.push_back(std::stoul(val, nullptr, 16));[m
[32m+[m[32m        }[m
[32m+[m
[32m+[m[32m        for (int i = 0; i < std::min((int)vecPalette.size(), 16) ; i++) {[m
[32m+[m[32m            ledPalette[i] = vecPalette[i];[m
[32m+[m[32m        }[m
[32m+[m
[32m+[m[32m        split(obj, "\"vmem_settings\": [ ", pos);[m
[32m+[m
[32m+[m[32m        int vmemArr[14];[m
[32m+[m[32m        {[m
[32m+[m[32m            auto dat = split(obj, " ]", pos);[m
[32m+[m[32m            std::stringstream s(dat);[m
[32m+[m[32m            std::string val;[m
[32m+[m[32m            for (size_t i = 0; i < 14; i++) {[m
[32m+[m[32m                std::getline(s, val, ',');[m
[32m+[m[32m                vmemArr[i] = atoi(val.c_str());[m
[32m+[m[32m            }[m
[32m+[m[32m        }[m
[32m+[m
[32m+[m[32m        vmAddr.numBits = std::max(0, std::min(vmemArr[0], 32));[m
[32m+[m[32m        vmAddr.pos.x = vmemArr[1];[m
[32m+[m[32m        vmAddr.pos.y = vmemArr[2];[m
[32m+[m[32m        vmAddr.stride.x = -vmemArr[3];[m
[32m+[m[32m        vmAddr.stride.y = vmemArr[4];[m
[32m+[m[32m        vmAddr.size.x = vmemArr[5];[m
[32m+[m[32m        vmAddr.size.y = vmemArr[6];[m
[32m+[m
[32m+[m[32m        vmData.numBits = std::max(0, std::min(vmemArr[7], 32));[m
[32m+[m[32m        vmData.pos.x = vmemArr[8];[m
[32m+[m[32m        vmData.pos.y = vmemArr[9];[m
[32m+[m[32m        vmData.stride.x = -vmemArr[10];[m
[32m+[m[32m        vmData.stride.y = vmemArr[11];[m
[32m+[m[32m        vmData.size.x = vmemArr[12];[m
[32m+[m[32m        vmData.size.y = vmemArr[13];[m
[32m+[m
[32m+[m[32m        vmWe.numBits = 0;[m
[32m+[m[32m        vmWe.gids[0] = -1;[m
[32m+[m
[32m+[m[32m        if (vmemFlag) {[m
[32m+[m[32m            vmemSize = 1ull << vmAddr.numBits;[m
[32m+[m[32m            vmem = new int[vmemSize];[m
[32m+[m[32m            memset(vmem, 0, 4 * vmemSize);[m
[32m+[m[32m        }[m
[32m+[m
[32m+[m[32m        if (!Project::processLogicData(logicData, 24)) {[m
[32m+[m[32m            return false;[m
[32m+[m[32m        }[m
[32m+[m
[32m+[m[32m        if (vmemFlag) {[m
[32m+[m[32m            for (int i = 0; i < vmAddr.numBits; i++) {[m
[32m+[m[32m                auto start = vmAddr.pos + i * vmAddr.stride;[m
[32m+[m[32m                auto end = start + vmAddr.size;[m
[32m+[m[32m                for (auto p = start; p.x < end.x; p.x++) {[m
[32m+[m[32m                    for (p.y = start.y; p.y < end.y; p.y++) {[m
[32m+[m[32m                        if (p.x < 0 || p.x >= width || p.y < 0 || p.y >= height)[m
[32m+[m[32m                            continue;[m
[32m+[m[32m                        image[p.x + p.y * width].ink = (int16_t)Ink::LatchOff;[m
[32m+[m[32m                    }[m
[32m+[m[32m                }[m
[32m+[m[32m            }[m
[32m+[m
[32m+[m[32m            for (int i = 0; i < vmData.numBits; i++) {[m
[32m+[m[32m                auto start = vmData.pos + i * vmData.stride;[m
[32m+[m[32m                auto end = start + vmData.size;[m
[32m+[m[32m                for (auto p = start; p.x < end.x; p.x++) {[m
[32m+[m[32m                    for (p.y = start.y; p.y < end.y; p.y++) {[m
[32m+[m[32m                        if (p.x < 0 || p.x >= width || p.y < 0 || p.y >= height)[m
[32m+[m[32m                            continue;[m
[32m+[m[32m                        image[p.x + p.y * width].ink = (int16_t)Ink::LatchOff;[m
[32m+[m[32m                    }[m
[32m+[m[32m                }[m
[32m+[m[32m            }[m
[32m+[m[32m        }[m
[32m+[m
[32m+[m[32m        Project::processDecorationData(decorationData[0], decoration[0]);[m
[32m+[m[32m        Project::processDecorationData(decorationData[1], decoration[1]);[m
[32m+[m[32m        Project::processDecorationData(decorationData[2], decoration[2]);[m
[32m+[m[32m        return true;[m
[32m+[m[32m    }[m
 }[m
\ No newline at end of file[m
[1mdiff --git a/tests/__pycache__/run_tests.cpython-312.pyc b/tests/__pycache__/run_tests.cpython-312.pyc[m
[1mdeleted file mode 100644[m
[1mindex 1470a25..0000000[m
Binary files a/tests/__pycache__/run_tests.cpython-312.pyc and /dev/null differ
[1mdiff --git a/tests/run_tests.py.bak b/tests/run_tests.py.bak[m
[1mdeleted file mode 100755[m
[1mindex ddd1800..0000000[m
[1m--- a/tests/run_tests.py.bak[m
[1m+++ /dev/null[m
[36m@@ -1,468 +0,0 @@[m
[31m-#!/usr/bin/env python3[m
[31m-import base64[m
[31m-import json[m
[31m-import os[m
[31m-import re[m
[31m-import shutil[m
[31m-import struct[m
[31m-import subprocess[m
[31m-import sys[m
[31m-[m
[31m-REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))[m
[31m-BIN = os.path.join(REPO, "build", "vcb2verilog")[m
[31m-TMP_DIR = "/tmp/vcb2verilog_tests"[m
[31m-[m
[31m-COLORS = {[m
[31m-    ".": None,[m
[31m-    "Trace": 0x5A5845,[m
[31m-    "Trace1": 0x9FA8AE,[m
[31m-    "Trace2": 0xA1555E,[m
[31m-    "Trace3": 0xA16C56,[m
[31m-    "Trace4": 0x6CA156,[m
[31m-    "Trace5": 0x5693A1,[m
[31m-    "R": 0x2E475D,[m
[31m-    "W": 0x4D383E,[m
[31m-    "Cross": 0x66788E,[m
[31m-    "Buffer": 0x3E4D3E,[m
[31m-    "OR": 0x384B56,[m
[31m-    "NAND": 0x4D392F,[m
[31m-    "NOT": 0x4D3744,[m
[31m-    "NOR": 0x1A3C56,[m
[31m-    "AND": 0x4D453E,[m
[31m-    "XOR": 0x433D56,[m
[31m-    "NXOR": 0x3B2854,[m
[31m-    "XNOR": 0x3B2854,[m
[31m-    "Clock": 0x4D243C,[m
[31m-    "Latch": 0x384D47,[m
[31m-    "LED": 0x323841,[m
[31m-    "BUS": 0x6C9799,[m
[31m-    "BUS0": 0x7A2F24,[m
[31m-    "BUS1": 0x3E7A24,[m
[31m-    "BUS2": 0x24417A,[m
[31m-    "BUS3": 0x25627A,[m
[31m-    "BUS4": 0x7A2D66,[m
[31m-    "BUS5": 0x7A7024,[m
[31m-    "Tunnel": 0x535572,[m
[31m-    "Mesh": 0x646A57,[m
[31m-}[m
[31m-[m
[31m-[m
[31m-def compress_zstd(raw: bytes) -> bytes:[m
[31m-    try:[m
[31m-        import zstandard as zstd  # type: ignore[m
[31m-        return zstd.ZstdCompressor().compress(raw)[m
[31m-    except ImportError:[m
[31m-        p = subprocess.run([m
[31m-            ["zstd", "-c", "-q"],[m
[31m-            input=raw,[m
[31m-            capture_output=True,[m
[31m-            check=True,[m
[31m-        )[m
[31m-        return p.stdout[m
[31m-[m
[31m-[m
[31m-def build_vcb(grid_rows: list[list[str]], out_path: str) -> None:[m
[31m-    height = len(grid_rows)[m
[31m-    width = len(grid_rows[0])[m
[31m-    for r in grid_rows:[m
[31m-        assert len(r) == width, f"Nekonzistentní šířka řádku: {len(r)} != {width}"[m
[31m-[m
[31m-    buf = bytearray(width * height * 4)[m
[31m-    for y, row in enumerate(grid_rows):[m
[31m-        for x, token in enumerate(row):[m
[31m-            if token not in COLORS:[m
[31m-                raise ValueError(f"Neznámý prvek '{token}' na ({x},{y})")[m
[31m-            rgb = COLORS[token][m
[31m-            idx = (y * width + x) * 4[m
[31m-            if rgb is None:[m
[31m-                buf[idx : idx + 4] = b"\x00\x00\x00\x00"[m
[31m-            else:[m
[31m-                r = (rgb >> 16) & 0xFF[m
[31m-                g = (rgb >> 8) & 0xFF[m
[31m-                b = rgb & 0xFF[m
[31m-                buf[idx : idx + 4] = struct.pack("BBBB", r, g, b, 255)[m
[31m-[m
[31m-    comp = compress_zstd(bytes(buf))[m
[31m-    header = struct.pack("<6i", 2, height, 2, width, 2, width * height * 4)[m
[31m-    layer0 = base64.b64encode(comp + header).decode("ascii")[m
[31m-[m
[31m-    doc = {[m
[31m-        "file_type": "RF_VCB_PROJECT",[m
[31m-        "file_version": 2,[m
[31m-        "title": "",[m
[31m-        "creation_date": 0,[m
[31m-        "last_edit_date": 0,[m
[31m-        "time_worked_on": 0,[m
[31m-        "layers": [layer0],[m
[31m-        "assembly": "",[m
[31m-        "is_vmem_enabled": False,[m
[31m-        "vmem_settings": [0] * 16,[m
[31m-        "camera_position": [0.0, 0.0],[m
[31m-        "camera_zoom": 1.0,[m
[31m-        "led_palette": ["0x323841", "0xffffff"],[m
[31m-        "docking_sizes": [0, 0, 0, 0],[m
[31m-        "docking_collapseness": [False] * 6,[m
[31m-        "docking_sidepanels": ["", "", "", ""],[m
[31m-        "assembly_is_external": False,[m
[31m-        "vmem_is_external": False,[m
[31m-        "notes": "",[m
[31m-        "clock_interval": 1,[m
[31m-        "timer_interval": 500,[m
[31m-        "random_seed": 1,[m
[31m-        "random_is_time_seed": True,[m
[31m-        "vdisplay_is_enabled": False,[m
[31m-        "vdisplay_is_visible": True,[m
[31m-        "vdisplay_settings": [0] * 8,[m
[31m-        "vdisplay_color_depth": 1,[m
[31m-        "vdisplay_direction": 0,[m
[31m-        "vdisplay_palette": ["", ""],[m
[31m-        "vinput_is_enabled": False,[m
[31m-        "vinput_settings": [0] * 7,[m
[31m-        "vinput_mode": 0,[m
[31m-        "vinput_bindings": "",[m
[31m-        "vmem_data": "",[m
[31m-        "decoration_palette": [""] * 32,[m
[31m-    }[m
[31m-[m
[31m-    with open(out_path, "w", encoding="utf-8") as f:[m
[31m-        json.dump(doc, f)[m
[31m-[m
[31m-[m
[31m-def parse_ports(verilog_path: str):[m
[31m-    with open(verilog_path, "r", encoding="utf-8") as f:[m
[31m-        code = f.read()[m
[31m-[m
[31m-    mod_m = re.search(r"\bmodule\s+([A-Za-z0-9_]+)\s*\(", code)[m
[31m-    assert mod_m, "Nepodařilo se najít název modulu ve Verilogu"[m
[31m-    mod_name = mod_m.group(1)[m
[31m-[m
[31m-    def coords(name: str):[m
[31m-        m = re.search(r"_(\d+)_(\d+)$", name)[m
[31m-        return (int(m.group(1)), int(m.group(2))) if m else (0, 0)[m
[31m-[m
[31m-    inputs = sorted(set(re.findall(r"\binput\s+(?:wire\s+)?([A-Za-z0-9_]+)", code)), key=coords)[m
[31m-    outputs = sorted(set(re.findall(r"\boutput\s+(?:wire\s+)?([A-Za-z0-9_]+)", code)), key=coords)[m
[31m-    return mod_name, inputs, outputs, code[m
[31m-[m
[31m-[m
[31m-def run_yosys_check(verilog_path: str) -> None:[m
[31m-    cmd = [[m
[31m-        "yosys",[m
[31m-        "-Q",[m
[31m-        "-p",[m
[31m-        f"read_verilog {verilog_path}; hierarchy -auto-top; proc; check; opt",[m
[31m-    ][m
[31m-    res = subprocess.run(cmd, capture_output=True, text=True)[m
[31m-    if res.returncode != 0:[m
[31m-        raise RuntimeError(f"Yosys selhal:\n{res.stdout}\n{res.stderr}")[m
[31m-[m
[31m-[m
[31m-def simulate_comb(verilog_path: str, test_name: str, expected_table: list[tuple[tuple[int, ...], tuple[int, ...]]]) -> None:[m
[31m-    mod_name, inputs, outputs, _ = parse_ports(verilog_path)[m
[31m-    sw_inputs = [p for p in inputs if p.startswith("sw_")][m
[31m-    other_inputs = [p for p in inputs if not p.startswith("sw_")][m
[31m-    if other_inputs:[m
[31m-        raise AssertionError(f"[{test_name}] Neočekávané extra vstupy v modulu: {other_inputs}")[m
[31m-[m
[31m-    tb_path = os.path.join(TMP_DIR, f"{test_name}_tb.v")[m
[31m-    vvp_path = os.path.join(TMP_DIR, f"{test_name}.vvp")[m
[31m-[m
[31m-    lines = [[m
[31m-        "`timescale 1ns/1ps",[m
[31m-        "module tb;",[m
[31m-    ][m
[31m-    for inp in sw_inputs:[m
[31m-        lines.append(f"  reg {inp} = 0;")[m
[31m-    for out in outputs:[m
[31m-        lines.append(f"  wire {out};")[m
[31m-[m
[31m-    port_conns = ", ".join(f".{p}({p})" for p in sw_inputs + outputs)[m
[31m-    lines.append(f"  {mod_name} dut ({port_conns});")[m
[31m-    lines.append("  initial begin")[m
[31m-[m
[31m-    for idx, (in_vals, out_vals) in enumerate(expected_table):[m
[31m-        assert len(in_vals) == len(sw_inputs), f"Počet vstupů nesedí: {len(in_vals)} vs {sw_inputs}"[m
[31m-        assert len(out_vals) == len(outputs), f"Počet výstupů nesedí: {len(out_vals)} vs {outputs}"[m
[31m-        for name, val in zip(sw_inputs, in_vals):[m
[31m-            lines.append(f"    {name} = 1'b{val};")[m
[31m-        lines.append("    #10;")[m
[31m-        for name, exp in zip(outputs, out_vals):[m
[31m-            lines.append([m
[31m-                f'    if ({name} !== 1\'b{exp}) begin '[m
[31m-                f'$display("FAIL_CASE {idx} {name}=%b expected={exp}", {name}); $finish_and_return(1); end'[m
[31m-            )[m
[31m-    lines.append('    $display("SIM_OK");')[m
[31m-    lines.append("    $finish;")[m
[31m-    lines.append("  end")[m
[31m-    lines.append("endmodule")[m
[31m-[m
[31m-    with open(tb_path, "w", encoding="utf-8") as f:[m
[31m-        f.write("\n".join(lines) + "\n")[m
[31m-[m
[31m-    c = subprocess.run(["iverilog", "-o", vvp_path, verilog_path, tb_path], capture_output=True, text=True)[m
[31m-    if c.returncode != 0:[m
[31m-        raise RuntimeError(f"iverilog selhal:\n{c.stderr}")[m
[31m-[m
[31m-    s = subprocess.run(["vvp", vvp_path], capture_output=True, text=True)[m
[31m-    if s.returncode != 0 or "SIM_OK" not in s.stdout:[m
[31m-        raise RuntimeError(f"Simulace selhala:\n{s.stdout}\n{s.stderr}")[m
[31m-[m
[31m-[m
[31m-def simulate_seq_buffer(verilog_path: str, test_name: str) -> None:[m
[31m-    mod_name, inputs, outputs, _ = parse_ports(verilog_path)[m
[31m-    assert "clk" in inputs, f"[{test_name}] Chybí port 'clk', nalezené vstupy: {inputs}"[m
[31m-    sw_inputs = [p for p in inputs if p.startswith("sw_")][m
[31m-    assert len(sw_inputs) == 1 and len(outputs) == 1[m
[31m-    sw = sw_inputs[0][m
[31m-    led = outputs[0][m
[31m-    has_rst = "rst" in inputs[m
[31m-[m
[31m-    tb_path = os.path.join(TMP_DIR, f"{test_name}_tb.v")[m
[31m-    vvp_path = os.path.join(TMP_DIR, f"{test_name}.vvp")[m
[31m-[m
[31m-    conns = [f".clk(clk)", f".{sw}({sw})", f".{led}({led})"][m
[31m-    if has_rst:[m
[31m-        conns.append(".rst(rst)")[m
[31m-[m
[31m-    tb = rf"""`timescale 1ns/1ps[m
[31m-module tb;[m
[31m-  reg clk = 0;[m
[31m-  {"reg rst = 1;" if has_rst else ""}[m
[31m-  reg {sw} = 0;[m
[31m-  wire {led};[m
[31m-  {mod_name} dut ({", ".join(conns)});[m
[31m-  always #5 clk = ~clk;[m
[31m-  initial begin[m
[31m-    {"#12 rst = 0;" if has_rst else "#12;"}[m
[31m-    if ({led} !== 1'b0) begin $display("FAIL init %b", {led}); $finish_and_return(1); end[m
[31m-    {sw} = 1'b1;[m
[31m-    #2;[m
[31m-    if ({led} !== 1'b0) begin $display("FAIL before posedge %b", {led}); $finish_and_return(1); end[m
[31m-    @(posedge clk); @(posedge clk); @(posedge clk); #2;[m
[31m-    if ({led} !== 1'b1) begin $display("FAIL after posedge %b", {led}); $finish_and_return(1); end[m
[31m-    $display("SIM_OK");[m
[31m-    $finish;[m
[31m-  end[m
[31m-endmodule[m
[31m-"""[m
[31m-    with open(tb_path, "w", encoding="utf-8") as f:[m
[31m-        f.write(tb)[m
[31m-[m
[31m-    c = subprocess.run(["iverilog", "-o", vvp_path, verilog_path, tb_path], capture_output=True, text=True)[m
[31m-    if c.returncode != 0:[m
[31m-        raise RuntimeError(f"iverilog selhal:\n{c.stderr}")[m
[31m-    s = subprocess.run(["vvp", vvp_path], capture_output=True, text=True)[m
[31m-    if s.returncode != 0 or "SIM_OK" not in s.stdout:[m
[31m-        raise RuntimeError(f"Sekvenční simulace selhala:\n{s.stdout}\n{s.stderr}")[m
[31m-[m
[31m-[m
[31m-def compile_vcb(vcb_path: str, out_v: str) -> None:[m
[31m-    res = subprocess.run([BIN, vcb_path, "-o", out_v], capture_output=True, text=True)[m
[31m-    if res.returncode != 0:[m
[31m-        raise RuntimeError(f"vcb2verilog selhal:\n{res.stdout}\n{res.stderr}")[m
[31m-[m
[31m-[m
[31m-def main() -> int:[m
[31m-    if not os.path.exists(BIN):[m
[31m-        print(f"[FAIL] Binárka nenalezena: {BIN}")[m
[31m-        return 1[m
[31m-[m
[31m-    if os.path.exists(TMP_DIR):[m
[31m-        shutil.rmtree(TMP_DIR)[m
[31m-    os.makedirs(TMP_DIR, exist_ok=True)[m
[31m-[m
[31m-    tests = [][m
[31m-[m
[31m-    def test_smoke_counter():[m
[31m-        out_v = os.path.join(TMP_DIR, "counter_2bit.v")[m
[31m-        compile_vcb(os.path.join(REPO, "examples", "counter_2bit.vcb"), out_v)[m
[31m-        run_yosys_check(out_v)[m
[31m-[m
[31m-    tests.append(("1_smoke_counter_2bit", test_smoke_counter))[m
[31m-[m
[31m-    def test_all_logic_gates():[m
[31m-        w = 26[m
[31m-        grid = [["."] * w for _ in range(9)][m
[31m-        grid[0][0] = "Latch"[m
[31m-        grid[1][0] = "W"[m
[31m-        for x in range(w):[m
[31m-            grid[2][x] = "Trace"[m
[31m-        for x in range(w):[m
[31m-            grid[6][x] = "Trace1"[m
[31m-        grid[7][0] = "W"[m
[31m-        grid[8][0] = "Latch"[m
[31m-[m
[31m-        gates = ["AND", "OR", "XOR", "NAND", "NOR", "NXOR"][m
[31m-        for i, g in enumerate(gates):[m
[31m-            gx = 2 + i * 4[m
[31m-            grid[3][gx] = "R"[m
[31m-            grid[4][gx] = g[m
[31m-            grid[5][gx] = "R"[m
[31m-            grid[4][gx + 1] = "W"[m
[31m-            grid[4][gx + 2] = "R"[m
[31m-            grid[4][gx + 3] = "LED"[m
[31m-[m
[31m-        vcb = os.path.join(TMP_DIR, "all_gates.vcb")[m
[31m-        out_v = os.path.join(TMP_DIR, "all_gates.v")[m
[31m-        build_vcb(grid, vcb)[m
[31m-        compile_vcb(vcb, out_v)[m
[31m-        run_yosys_check(out_v)[m
[31m-        table = [[m
[31m-            ((0, 0), (0, 0, 0, 0, 1, 1)),[m
[31m-            ((0, 1), (0, 1, 1, 1, 0, 0)),[m
[31m-            ((1, 0), (0, 1, 1, 1, 0, 0)),[m
[31m-            ((1, 1), (1, 1, 0, 1, 0, 1)),[m
[31m-        ][m
[31m-        simulate_comb(out_v, "all_gates", table)[m
[31m-[m
[31m-    tests.append(("2_all_two_input_gates", test_all_logic_gates))[m
[31m-[m
[31m-    def test_not_chain():[m
[31m-        grid = [[m
[31m-            ["Latch", "W", "Trace", "R", "NOT", "W", "Trace1", "R", "LED", "."],[m
[31m-            [".",     ".", ".",     ".", ".",   ".", "R",      ".", ".",   "."],[m
[31m-            [".",     ".", ".",     ".", ".",   ".", "NOT",    "W", "R",   "LED"],[m
[31m-        ][m
[31m-        vcb = os.path.join(TMP_DIR, "not_chain.vcb")[m
[31m-        out_v = os.path.join(TMP_DIR, "not_chain.v")[m
[31m-        build_vcb(grid, vcb)[m
[31m-        compile_vcb(vcb, out_v)[m
[31m-        run_yosys_check(out_v)[m
[31m-        table = [[m
[31m-            ((0,), (1, 0)),[m
[31m-            ((1,), (0, 1)),[m
[31m-        ][m
[31m-        simulate_comb(out_v, "not_chain", table)[m
[31m-[m
[31m-    tests.append(("3_not_and_double_negation", test_not_chain))[m
[31m-[m
[31m-    def test_no_spurious_inputs():[m
[31m-        grid = [[m
[31m-            ["Latch", "W", "Trace", "R", "LED"],[m
[31m-            [".",     ".", ".",     ".", "."],[m
[31m-            ["R",     "Trace1", ".", ".", "."],[m
[31m-            ["BUS2",  "R", ".",     ".", "."],[m
[31m-        ][m
[31m-        vcb = os.path.join(TMP_DIR, "floating_r.vcb")[m
[31m-        out_v = os.path.join(TMP_DIR, "floating_r.v")[m
[31m-        build_vcb(grid, vcb)[m
[31m-        compile_vcb(vcb, out_v)[m
[31m-        run_yosys_check(out_v)[m
[31m-        _, inputs, outputs, _ = parse_ports(out_v)[m
[31m-        assert len(inputs) == 1 and inputs[0].startswith("sw_"), f"Falešné vstupy: {inputs}"[m
[31m-        assert len(outputs) == 1[m
[31m-        simulate_comb(out_v, "floating_r", [((0,), (0,)), ((1,), (1,))])[m
[31m-[m
[31m-    tests.append(("4_no_spurious_input_ports", test_no_spurious_inputs))[m
[31m-[m
[31m-    def test_bus_channel_isolation():[m
[31m-        grid = [[m
[31m-            ["Latch", "W", "BUS2", ".",    ".",   ".", "."],[m
[31m-            [".",     ".", "BUS2", "R",    "AND", "W", "LED"],[m
[31m-            ["Latch", "W", "R",    "BUS2", ".",   ".", "."],[m
[31m-            [".",     ".", "BUS2", "W",    "R",   "LED", "."],[m
[31m-        ][m
[31m-        vcb = os.path.join(TMP_DIR, "bus_iso.vcb")[m
[31m-        out_v = os.path.join(TMP_DIR, "bus_iso.v")[m
[31m-        build_vcb(grid, vcb)[m
[31m-        compile_vcb(vcb, out_v)[m
[31m-        run_yosys_check(out_v)[m
[31m-        _, inputs, outputs, _ = parse_ports(out_v)[m
[31m-        assert "clk" not in inputs and "rst" not in inputs, f"Nechtěné clk/rst: {inputs}"[m
[31m-        assert len(outputs) == 2[m
[31m-[m
[31m-    tests.append(("5_bus_channel_isolation", test_bus_channel_isolation))[m
[31m-[m
[31m-    def test_3x22_half_adder():[m
[31m-        grid = [[m
[31m-            ["LED",   ".",    "LED"],[m
[31m-            ["R",     ".",    "R"],[m
[31m-            ["W",     ".",    "W"],[m
[31m-            ["BUS2",  "R",    "XOR"],[m
[31m-            ["BUS2",  ".",    "."],[m
[31m-            ["BUS2",  ".",    "AND"],[m
[31m-            ["BUS2",  ".",    "R"],[m
[31m-            ["BUS2",  "R",    "."],[m
[31m-            ["BUS2",  "AND",  "R"],[m
[31m-            ["BUS2",  "AND",  "Buffer"],[m
[31m-            ["BUS2",  "W",    "AND"],[m
[31m-            ["BUS2",  "BUS2", "R"],[m
[31m-            ["BUS2",  "W",    "AND"],[m
[31m-            ["BUS2",  "BUS2", "."],[m
[31m-            ["BUS2",  "W",    "AND"],[m
[31m-            ["BUS2",  "BUS2", "R"],[m
[31m-            ["BUS2",  ".",    "W"],[m
[31m-            ["W",     "BUS2", "XOR"],[m
[31m-            ["AND",   "BUS2", "AND"],[m
[31m-            ["R",     "XOR",  "R"],[m
[31m-            ["W",     ".",    "W"],[m
[31m-            ["Latch", ".",    "Latch"],[m
[31m-        ][m
[31m-        vcb = os.path.join(TMP_DIR, "test2_3x22.vcb")[m
[31m-        out_v = os.path.join(TMP_DIR, "test2_3x22.v")[m
[31m-        build_vcb(grid, vcb)[m
[31m-        compile_vcb(vcb, out_v)[m
[31m-        run_yosys_check(out_v)[m
[31m-        _, inputs, _, _ = parse_ports(out_v)[m
[31m-        assert "clk" not in inputs and "rst" not in inputs, f"Nechtěné clk/rst: {inputs}"[m
[31m-        table = [[m
[31m-            ((0, 0), (0, 0)),[m
[31m-            ((0, 1), (1, 1)),[m
[31m-            ((1, 0), (1, 1)),[m
[31m-            ((1, 1), (1, 0)),[m
[31m-        ][m
[31m-        simulate_comb(out_v, "test2_3x22", table)[m
[31m-[m
[31m-    tests.append(("6_3x22_bus2_xor_or_regression", test_3x22_half_adder))[m
[31m-[m
[31m-    def test_buffer_dff():[m
[31m-        grid = [[m
[31m-            ["Latch", "W", "Trace", "R", "Buffer", "W", "Trace1", "R", "LED"],[m
[31m-        ][m
[31m-        vcb = os.path.join(TMP_DIR, "buffer_dff.vcb")[m
[31m-        out_v = os.path.join(TMP_DIR, "buffer_dff.v")[m
[31m-        build_vcb(grid, vcb)[m
[31m-        subprocess.run([BIN, vcb, "-o", out_v, "--clock", "on"], check=True, capture_output=True)[m
[31m-        run_yosys_check(out_v)[m
[31m-        simulate_seq_buffer(out_v, "buffer_dff")[m
[31m-[m
[31m-    tests.append(("7_sequential_buffer_dff", test_buffer_dff))[m
[31m-[m
[31m-    def test_wired_or():[m
[31m-        grid = [[m
[31m-            ["Latch", "W", "Trace", "R", "LED"],[m
[31m-            [".",     ".", "Trace", ".", "."],[m
[31m-            ["Latch", "W", "Trace", ".", "."],[m
[31m-        ][m
[31m-        vcb = os.path.join(TMP_DIR, "wired_or.vcb")[m
[31m-        out_v = os.path.join(TMP_DIR, "wired_or.v")[m
[31m-        build_vcb(grid, vcb)[m
[31m-        compile_vcb(vcb, out_v)[m
[31m-        run_yosys_check(out_v)[m
[31m-        table = [[m
[31m-            ((0, 0), (0,)),[m
[31m-            ((0, 1), (1,)),[m
[31m-            ((1, 0), (1,)),[m
[31m-            ((1, 1), (1,)),[m
[31m-        ][m
[31m-        simulate_comb(out_v, "wired_or", table)[m
[31m-[m
[31m-    tests.append(("8_wired_or_multi_write", test_wired_or))[m
[31m-[m
[31m-    passed = 0[m
[31m-    failed = 0[m
[31m-    for name, fn in tests:[m
[31m-        try:[m
[31m-            fn()[m
[31m-            print(f"[PASS] {name}")[m
[31m-            passed += 1[m
[31m-        except Exception as e:[m
[31m-            print(f"[FAIL] {name}: {e}")[m
[31m-            failed += 1[m
[31m-[m
[31m-    print(f"\nSouhrn: {passed}/{len(tests)} testů prošlo.")[m
[31m-    return 0 if failed == 0 else 1[m
[31m-[m
[31m-[m
[31m-if __name__ == "__main__":[m
[31m-    sys.exit(main())[m
