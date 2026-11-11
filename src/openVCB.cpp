// openVCB - Virtual Circuit Board utility methods
// SPDX-License-Identifier: GPL-3.0-or-later

#include "openVCB.h"

namespace openVCB {
	using namespace std;
	using namespace glm;

	void Project::toggleLatch(ivec2 pos) {
		if (pos.x < 0 || pos.x >= width ||
			pos.y < 0 || pos.y >= height)
			return;
		const int gid = indexImage[pos.x + pos.y * width];
		toggleLatch(gid);
	}

	void Project::toggleLatch(int gid) {
		if (gid < 0 || gid >= numGroups) return;
		if (setOff(stateInks[gid]) != Ink::LatchOff)
			return;
		states[gid].activeInputs = 1;
	}

	Project::~Project() {
		if (originalImage) delete[] originalImage;
		if (vmem) delete[] vmem;
		if (image) delete[] image;
		if (indexImage) delete[] indexImage;
		if (decoration[0]) delete[] decoration[0];
		if (decoration[1]) delete[] decoration[1];
		if (decoration[2]) delete[] decoration[2];
		if (writeMap.ptr) delete[] writeMap.ptr;
		if (writeMap.rows) delete[] writeMap.rows;
		if (states) delete[] states;
		if (stateInks) delete[] stateInks;
	}

	std::pair<Ink, int> Project::sample(ivec2 pos) {
		if (pos.x < 0 || pos.x >= width ||
			pos.y < 0 || pos.y >= height)
			return { Ink::None, -1 };

		int idx = pos.x + pos.y * width;
		Ink type = image[idx].getInk();
		idx = indexImage[idx];
		if (type == Ink::Cross) return { Ink::Cross, idx };

		if (idx == -1) return { Ink::None, -1 };

		unsigned char state = states[idx].logic;
		type = (Ink)(((int)type & 0x7f) | (state & 0x80));
		return { type, idx };
	}
}
