// Code for image proprocessing and graph generation

#include "openVCB.h"
#include <cstdio>
#include <unordered_set>
#include <algorithm>
#include <unordered_map>

#include "gorder/Graph.h"
#include "gorder/Util.h"

namespace openVCB {
    using namespace std;
    using namespace glm;

    const ivec2 fourNeighbors[4]{
        ivec2(-1, 0),
        ivec2(0, 1),
        ivec2(1, 0),
        ivec2(0, -1)
    };

    void exploreBundle(ivec2 pos, ivec2 entryDir, int mask, InkPixel* image, int width, int height,
        std::vector<int>& visited,
        std::vector<ivec2>& stack, std::vector<ivec2>& bundleStack,
        std::vector<ivec2>& readInks, std::vector<ivec2>& writeInks,
        const std::vector<ivec2>& meshPixels) {

        auto jumpTunnel = [&](ivec2 entryPos, ivec2 dir) {
            ivec2 tp = entryPos;
            while (tp.x >= 0 && tp.x < width && tp.y >= 0 && tp.y < height &&
                   image[tp.x + tp.y * width].getInk() == Ink::Tunnel) {
                visited[tp.x + tp.y * width] |= mask;
                tp += dir;
            }
            while (tp.x >= 0 && tp.x < width && tp.y >= 0 && tp.y < height) {
                int tidx = tp.x + tp.y * width;
                if (image[tidx].getInk() == Ink::Tunnel) {
                    if (!(visited[tidx] & mask)) {
                        visited[tidx] |= mask;
                        bundleStack.push_back(tp);
                    }
                    break;
                }
                tp += dir;
            }
        };

        Ink startInk = image[pos.x + pos.y * width].getInk();
        if (startInk == Ink::Tunnel) {
            jumpTunnel(pos, entryDir);
        } else {
            bundleStack.push_back(pos);
            visited[pos.x + pos.y * width] |= mask;
            if (startInk == Ink::Mesh) {
                for (const auto& mp : meshPixels) {
                    int midx = mp.x + mp.y * width;
                    if (midx >= 0 && midx < width * height && !(visited[midx] & mask)) {
                        visited[midx] |= mask;
                        bundleStack.push_back(mp);
                    }
                }
            }
        }

        while (bundleStack.size()) {
            const ivec2 p = bundleStack.back();
            bundleStack.pop_back();

            // Check four directions
            for (int k = 0; k < 4; k++) {
                ivec2 np = p + fourNeighbors[k];
                if (np.x < 0 || np.x >= width ||
                    np.y < 0 || np.y >= height) continue;

                int nidx = np.x + np.y * width;
                const int nvis = visited[nidx];
                // Check if already visited
                if (nvis & mask) continue;

                InkPixel newPix = image[nidx];
                Ink newInk = newPix.getInk();

                // Handle different inks
                if (setOff(newInk) == Ink::ReadOff) {
                    if (nvis & 1) continue;
                    readInks.push_back(np);
                    if ((mask >> 16) == 2) {
                        visited[nidx] |= 1;
                        stack.push_back(np);
                    }
                    continue;
                }
                else if (setOff(newInk) == Ink::WriteOff) {
                    if (nvis & 1) continue;
                    writeInks.push_back(np);
                    if ((mask >> 17) == 2) {
                        visited[nidx] |= 1;
                        stack.push_back(np);
                    }
                    continue;
                }
                else if (setOff(newInk) == Ink::TraceOff) {
                    if (nvis & 1) continue;
                    // We will only connect to traces of the matching color
                    if ((mask >> newPix.meta) == 2) {
                        visited[nidx] |= 1;
                        stack.push_back(np);
                    }
                    continue;
                }
                else if (newInk == Ink::Cross) {
                    np += fourNeighbors[k];
                    if (np.x < 0 || np.x >= width ||
                        np.y < 0 || np.y >= height) continue;

                    nidx = np.x + np.y * width;
                    if (visited[nidx] & mask) continue;
                    newInk = image[np.x + np.y * width].getInk();
                }

                if (newInk == Ink::Tunnel) {
                    jumpTunnel(np, fourNeighbors[k]);
                }
                else if (newInk == Ink::BundleOff || newInk == Ink::Mesh) {
                    visited[nidx] |= mask;
                    bundleStack.push_back(np);
                    if (newInk == Ink::Mesh) {
                        for (const auto& mp : meshPixels) {
                            int midx = mp.x + mp.y * width;
                            if (midx >= 0 && midx < width * height && !(visited[midx] & mask)) {
                                visited[midx] |= mask;
                                bundleStack.push_back(mp);
                            }
                        }
                    }
                }
            }
        }
    }

    void Project::preprocess(bool useGorder) {
        // Turn off any inks that start as off
#pragma omp parallel for schedule(static, 8192)
        for (int i = 0; i < width * height; i++) {
            Ink ink = (Ink)image[i].ink;
            switch (ink) {
            case Ink::Trace:
            case Ink::Read:
            case Ink::Write:
            case Ink::Buffer:
            case Ink::Or:
            case Ink::And:
            case Ink::Xor:
            case Ink::Not:
            case Ink::Nor:
            case Ink::Nand:
            case Ink::Xnor:
            case Ink::Clock:
            case Ink::Led:
            case Ink::Bundle:
                ink = setOff(ink);
                image[i].ink = (int16_t)ink;
            }
        }

        std::vector<int> visited(width * height, 0);
        std::vector<ivec2> stack;
        std::vector<ivec2> bundleStack;
        std::vector<ivec2> readInks;
        std::vector<ivec2> writeInks;

        writeMap.n = 0;

        indexImage = new int[width * height];
        for (int i = 0, lim = width * height; i < lim; i++)
            indexImage[i] = -1;

        std::vector<ivec2> meshPixels;
        for (int y = 0; y < height; y++)
            for (int x = 0; x < width; x++)
                if (image[x + y * width].getInk() == Ink::Mesh)
                    meshPixels.push_back(ivec2(x, y));

        using Group = tuple<int, Logic, Ink>;
        vector<Group> indexDict;

        unordered_multimap<int, int> bundleCons;
        unordered_set<long long> bundleConsSet;

        // Connected Components Search
        for (int y = 0; y < height; y++)
            for (int x = 0; x < width; x++)
                if (!(visited[x + y * width] & 1)) {
                    Ink ink = image[x + y * width].getInk();
                    if (ink == Ink::Cross || ink == Ink::None ||
                        ink == Ink::Annotation || ink == Ink::Filler)
                        continue;
                    else if (ink == Ink::ReadOff) {
                        readInks.push_back(ivec2(x, y));
                        ink = Ink::TraceOff;
                    }
                    else if (ink == Ink::WriteOff) {
                        writeInks.push_back(ivec2(x, y));
                        ink = Ink::TraceOff;
                    }

                    // Allocate new group id
                    const int gid = writeMap.n;
                    writeMap.n++;

                    // DFS
                    stack.push_back(ivec2(x, y));
                    visited[x + y * width] |= 1;
                    while (stack.size()) {
                        const ivec2 p = stack.back();
                        stack.pop_back();

                        const int idx = p.x + p.y * width;
                        indexImage[idx] = gid;

                        for (int k = 0; k < 4; k++) {
                            ivec2 np = p + fourNeighbors[k];
                            if (np.x < 0 || np.x >= width ||
                                np.y < 0 || np.y >= height) continue;

                            int nidx = np.x + np.y * width;
                            const int nvis = visited[nidx];

                            // Handle wire bundles, mesh, and tunnels
                            Ink newInk = image[nidx].getInk();
                            if (ink == Ink::TraceOff && (newInk == Ink::BundleOff || newInk == Ink::Mesh || newInk == Ink::Tunnel)) {
                                InkPixel npix = image[idx];
                                int mask;

                                if (setOff((Ink)npix.ink) == Ink::ReadOff)
                                    mask = 2 << 16;
                                else if (setOff((Ink)npix.ink) == Ink::WriteOff)
                                    mask = 2 << 17;
                                else
                                    mask = 2 << npix.meta;

                                if (nvis & mask) continue;

                                exploreBundle(np, fourNeighbors[k], mask, image, width, height, visited, stack, bundleStack, readInks, writeInks, meshPixels);

                                if (nvis & 1) {
                                    int otherIdx = indexImage[nidx];
                                    if (bundleConsSet.insert(((long long)otherIdx << 32) | gid).second)
                                        bundleCons.insert({ gid, otherIdx });
                                }

                                continue;
                            }

                            // Check if already visited
                            if (nvis & 1) {
                                if ((ink == Ink::BundleOff || ink == Ink::Mesh || ink == Ink::Tunnel) &&
                                    (newInk == Ink::TraceOff || newInk == Ink::ReadOff || newInk == Ink::WriteOff)) {
                                    int otherIdx = indexImage[nidx];
                                    if (bundleConsSet.insert(((long long)gid << 32) | otherIdx).second)
                                        bundleCons.insert({ otherIdx, gid });
                                }
                                continue;
                            }

                            // Check ink type and handle crosses
                            if (newInk == Ink::Cross) {
                                np += fourNeighbors[k];
                                if (np.x < 0 || np.x >= width ||
                                    np.y < 0 || np.y >= height) continue;

                                nidx = np.x + np.y * width;
                                if (visited[nidx] & 1) continue;
                                newInk = image[np.x + np.y * width].getInk();
                            }
                            // Push back if Allowable
                            if (setOff(newInk) == Ink::ReadOff && setOff(ink) == Ink::TraceOff) {
                                readInks.push_back(np);
                                visited[nidx] |= 1;
                                stack.push_back(np);
                            }
                            else if (setOff(newInk) == Ink::WriteOff && setOff(ink) == Ink::TraceOff) {
                                writeInks.push_back(np);
                                visited[nidx] |= 1;
                                stack.push_back(np);
                            }
                            else if (setOff(newInk) == setOff(ink)) {
                                visited[nidx] |= 1;
                                stack.push_back(np);
                            }
                            else if (ink != Ink::TraceOff && ink != Ink::BundleOff &&
                                     ink != Ink::Mesh && ink != Ink::Tunnel &&
                                     (newInk == Ink::BundleOff || newInk == Ink::Mesh || newInk == Ink::Tunnel)) {
                                // Merge same-type components connected via BUS / Mesh / Tunnel
                                const int gateBusBit = (1 << 20);
                                if (!(visited[nidx] & gateBusBit)) {
                                    std::vector<ivec2> gBusStack = { np };
                                    visited[nidx] |= gateBusBit;

                                    while (!gBusStack.empty()) {
                                        ivec2 bp = gBusStack.back();
                                        gBusStack.pop_back();

                                        for (int bk = 0; bk < 4; bk++) {
                                            ivec2 bnp = bp + fourNeighbors[bk];
                                            if (bnp.x < 0 || bnp.x >= width ||
                                                bnp.y < 0 || bnp.y >= height) continue;

                                            int bnidx = bnp.x + bnp.y * width;
                                            Ink bInk = image[bnidx].getInk();

                                            if (bInk == ink && !(visited[bnidx] & 1)) {
                                                visited[bnidx] |= 1;
                                                stack.push_back(bnp);
                                            }
                                            else if ((bInk == Ink::BundleOff || bInk == Ink::Mesh || bInk == Ink::Tunnel) &&
                                                     !(visited[bnidx] & gateBusBit)) {
                                                visited[bnidx] |= gateBusBit;
                                                gBusStack.push_back(bnp);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Add on the new group
                    indexDict.push_back({ gid, inkLogicType(setOff(ink)), setOff(ink) });
                }
        numGroups = writeMap.n;

        // Sort groups by ink vs. component then by morton code.
        std::sort(indexDict.begin(), indexDict.end(),
            [](const Group& a, const Group& b) -> bool {
                if (std::get<2>(a) == std::get<2>(b))
                    return std::get<0>(a) < std::get<0>(b);
                return (int)std::get<2>(a) < (int)std::get<2>(b);
            });

        // Build state vector
        states = new InkState[writeMap.n];
        stateInks = new Ink[writeMap.n];
        writeMap.ptr = new int[writeMap.n + 1];
        for (int i = 0; i < writeMap.n; i++) {
            auto g = indexDict[i];

            stateInks[i] = std::get<2>(g);

            InkState& s = states[i];
            s.logic = (unsigned char)std::get<1>(g);
            s.visited = 0;
            s.activeInputs = 0;

            writeMap.ptr[std::get<0>(g)] = i;
        }

        // Remap indices
        for (size_t i = 0, lim = width * height; i < lim; i++) {
            int idx = indexImage[i];
            if (idx >= 0)
                indexImage[i] = writeMap.ptr[idx];
        }

        // Hash sets to keep track of unique connections
        unordered_set<long long> conSet;
        std::vector<std::pair<int, int>> conList;

        // Add connections from inks to components
        for (ivec2 p : readInks) {
            const int srcGID = indexImage[p.x + p.y * width];

            for (int k = 0; k < 4; k++) {
                ivec2 np = p + fourNeighbors[k];
                if (np.x < 0 || np.x >= width ||
                    np.y < 0 || np.y >= height) continue;

                auto ink = image[np.x + np.y * width].ink;
                if (ink == (int16_t)Ink::BundleOff || ink == (int16_t)Ink::Mesh ||
                    ink == (int16_t)Ink::Tunnel || ink == (int16_t)Ink::ClockOff)
                    continue;

                const int dstGID = indexImage[np.x + np.y * width];
                if (srcGID != dstGID && dstGID != -1 && conSet.insert(((long long)srcGID << 32) | dstGID).second)
                    conList.push_back({ srcGID, dstGID });
            }
        }

        conSet.clear();

        // Add connections from components to inks
        for (ivec2 p : writeInks) {
            const int dstGID = indexImage[p.x + p.y * width];

            int oldTraceIdx = std::get<0>(indexDict[dstGID]);
            auto range = bundleCons.equal_range(oldTraceIdx);

            for (int k = 0; k < 4; k++) {
                ivec2 np = p + fourNeighbors[k];
                if (np.x < 0 || np.x >= width ||
                    np.y < 0 || np.y >= height) continue;

                auto ink = image[np.x + np.y * width].ink;
                if (ink == (int16_t)Ink::BundleOff || ink == (int16_t)Ink::Mesh || ink == (int16_t)Ink::Tunnel)
                    continue;

                const int srcGID = indexImage[np.x + np.y * width];
                if (srcGID != dstGID && srcGID != -1 && conSet.insert(((long long)srcGID << 32) | dstGID).second) {
                    conList.push_back({ srcGID, dstGID });

                    for (auto itr = range.first; itr != range.second; itr++) {
                        int bundleID = writeMap.ptr[itr->second];
                        if (conSet.insert(((long long)srcGID << 32) | bundleID).second)
                            conList.push_back({ srcGID, bundleID });
                    }
                }
            }
        }

        // Gorder
        if (useGorder) {
            Gorder::Graph g;
            vector<pair<int, int>> list(conList);
            g.readGraph(list, writeMap.n);

            std::vector<int> transformOrder;
            g.Transform(transformOrder);

            std::vector<int> order;
            g.GorderGreedy(order, 64);

            auto oldStates = states;
            states = new InkState[writeMap.n];
            for (int i = 0; i < writeMap.n; i++)
                states[order[transformOrder[i]]] = oldStates[i];
            delete[] oldStates;

            for (size_t i = 0; i < conList.size(); i++) {
                auto edge = conList[i];
                edge.first = order[transformOrder[edge.first]];
                edge.second = order[transformOrder[edge.second]];
                conList[i] = edge;
            }
            for (size_t i = 0; i < width * height; i++) {
                int idx = indexImage[i];
                if (idx >= 0)
                    indexImage[i] = order[transformOrder[idx]];
            }
        }

        // Stores rows per column
        std::vector<int> accu(writeMap.n, 0);
        for (auto e : conList)
            accu[e.first]++;

        // Construct adjacency matrix
        writeMap.nnz = conList.size();
        writeMap.ptr[writeMap.n] = writeMap.nnz;
        writeMap.rows = new int[writeMap.nnz];

        // Prefix sum
        int c = 0;
        for (size_t i = 0; i < writeMap.n; i++) {
            writeMap.ptr[i] = c;
            c += accu[i];
            accu[i] = 0;
        }

        // Populate
        for (int i = 0; i < conList.size(); i++) {
            auto con = conList[i];

            Ink dstInk = stateInks[con.second];
            if (dstInk == Ink::AndOff ||
                dstInk == Ink::NandOff)
                states[con.second].activeInputs--;

            writeMap.rows[writeMap.ptr[con.first] + (accu[con.first]++)] = con.second;
        }

        // Sort rows
        for (int i = 0; i < writeMap.n; i++) {
            int start = writeMap.ptr[i];
            int end = writeMap.ptr[i + 1];
            std::sort(&writeMap.rows[start], &writeMap.rows[end]);
        }

        // Record clocks and latch initial states
        for (size_t i = 0; i < writeMap.n; i++) {
            Ink ink = stateInks[i];
            if (ink == Ink::ClockOff)
                clockGIDs.push_back(i);

            if (ink == Ink::Latch)
                states[i].activeInputs = 1;
        }

        fprintf(stderr, "[DEBUG] Celkem nalezeno skupin (writeMap.n): %d, Spojení (conList): %zu\n", writeMap.n, conList.size());
    }
}