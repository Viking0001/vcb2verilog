// Code for image proprocessing and graph generation

#include "openVCB.h"
#include <cstdio>
#include <cstdint>
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
                else if (newInk == Ink::BundleOff || newInk == Ink::Bus || newInk == Ink::Mesh) {
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
            if (ink == Ink::Tunnel) {
			image[i].ink = (int16_t)Ink::BundleOff;
		} else {
			image[i].ink = (int16_t)setOff(ink);
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

        std::vector<int> busVisited(width * height, 0);
        int busGen = 0;

        // Connected Components Search
        for (int y = 0; y < height; y++)
            for (int x = 0; x < width; x++)
                if (!(visited[x + y * width] & 1)) {
                    Ink ink = image[x + y * width].getInk();
                    if (ink == Ink::Cross || ink == Ink::None || ink == Ink::Bus || ink == Ink::Mesh ||
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
                            if (ink == Ink::TraceOff && (newInk == Ink::BundleOff || newInk == Ink::Bus ||
                                                         newInk == Ink::Mesh || newInk == Ink::Tunnel)) {
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
                                continue;
                            }

                            // Check if already visited
                            if (nvis & 1) {
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
                            else if ((setOff(ink) == Ink::AndOff || setOff(ink) == Ink::OrOff || setOff(ink) == Ink::XorOff ||
                                      setOff(ink) == Ink::NotOff || setOff(ink) == Ink::NandOff || setOff(ink) == Ink::NorOff ||
                                      setOff(ink) == Ink::XnorOff || setOff(ink) == Ink::BufferOff) &&
                                     newInk == Ink::BundleOff) {
                                visited[nidx] |= 1;
                                stack.push_back(np);
                            }
                            else if (ink != Ink::TraceOff && ink != Ink::BundleOff &&
                                     ink != Ink::Mesh && ink != Ink::Tunnel &&
                                     (newInk == Ink::BundleOff || newInk == Ink::Tunnel)) {
                                // Preserve the existing Bundle/Tunnel merge
                                // behavior. BUS and Mesh identities are resolved
                                // deterministically after physical groups exist.
                                ++busGen;
                                if (busVisited[nidx] != busGen) {
                                    std::vector<ivec2> gBusStack = { np };
                                    busVisited[nidx] = busGen;

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
                                            else if ((bInk == Ink::BundleOff || bInk == Ink::Tunnel) &&
                                                     busVisited[bnidx] != busGen) {
                                                busVisited[bnidx] = busGen;
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

        // A BUS is a physical merge region, not a signal-graph node. First
        // discover each connected BUS region, then union the compatible
        // physical component groups touching that region. This pass runs
        // after all physical groups have been identified, so row-major scan
        // order cannot decide which group owns a BUS pixel.
        const int physicalGroupCount = writeMap.n;
        std::vector<int> parent(physicalGroupCount);
        std::vector<int> rank(physicalGroupCount, 0);
        for (int i = 0; i < physicalGroupCount; ++i) parent[i] = i;

        auto findRoot = [&](int value) {
            int root = value;
            while (parent[root] != root) root = parent[root];
            while (parent[value] != value) {
                int next = parent[value];
                parent[value] = root;
                value = next;
            }
            return root;
        };
        auto uniteGroups = [&](int first, int second) {
            int a = findRoot(first);
            int b = findRoot(second);
            if (a == b) return;
            if (rank[a] < rank[b]) std::swap(a, b);
            parent[b] = a;
            if (rank[a] == rank[b]) ++rank[a];
        };
        auto busMergeType = [](Ink ink) {
            ink = setOff(ink);
            return ink != Ink::TraceOff && ink != Ink::BundleOff &&
                   ink != Ink::Bus && ink != Ink::Mesh && ink != Ink::Cross && ink != Ink::None &&
                   ink != Ink::Filler && ink != Ink::Annotation;
        };

        std::vector<unsigned char> busRegionVisited(width * height, 0);
        std::vector<ivec2> physicalBusStack;
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int startIdx = x + y * width;
                if (image[startIdx].getInk() != Ink::Bus || busRegionVisited[startIdx]) continue;

                std::unordered_map<int, int> firstGroupByType;
                physicalBusStack.push_back(ivec2(x, y));
                busRegionVisited[startIdx] = 1;
                while (!physicalBusStack.empty()) {
                    ivec2 p = physicalBusStack.back();
                    physicalBusStack.pop_back();

                    for (int k = 0; k < 4; ++k) {
                        ivec2 np = p + fourNeighbors[k];
                        if (np.x < 0 || np.x >= width || np.y < 0 || np.y >= height) continue;
                        int nidx = np.x + np.y * width;

                        // Cross keeps its existing straight-through behavior:
                        // a BUS can continue across it on the same axis, but
                        // the crossing itself does not connect the axes.
                        if (image[nidx].getInk() == Ink::Cross) {
                            np += fourNeighbors[k];
                            if (np.x < 0 || np.x >= width || np.y < 0 || np.y >= height) continue;
                            nidx = np.x + np.y * width;
                            if (image[nidx].getInk() != Ink::Bus) continue;
                        }

                        if (image[nidx].getInk() == Ink::Bus) {
                            if (!busRegionVisited[nidx]) {
                                busRegionVisited[nidx] = 1;
                                physicalBusStack.push_back(np);
                            }
                            continue;
                        }

                        int group = indexImage[nidx];
                        if (group < 0 || group >= physicalGroupCount) continue;
                        Ink componentType = std::get<2>(indexDict[group]);
                        if (!busMergeType(componentType)) continue;

                        int typeKey = static_cast<int>(setOff(componentType));
                        auto inserted = firstGroupByType.emplace(typeKey, group);
                        if (!inserted.second) uniteGroups(inserted.first->second, group);
                    }
                }
            }
        }

        // All Mesh pixels belong to one global merge resource, even when
        // physically separated. Union every compatible component touching
        // any Mesh pixel into the same per-ink disjoint sets.
        std::unordered_map<int, int> firstMeshGroupByType;
        for (const ivec2& meshPos : meshPixels) {
            for (int k = 0; k < 4; ++k) {
                ivec2 np = meshPos + fourNeighbors[k];
                if (np.x < 0 || np.x >= width || np.y < 0 || np.y >= height) continue;
                int nidx = np.x + np.y * width;

                // A Cross continues only along the axis on which this Mesh
                // reaches it; it does not connect the perpendicular axis.
                if (image[nidx].getInk() == Ink::Cross) {
                    np += fourNeighbors[k];
                    if (np.x < 0 || np.x >= width || np.y < 0 || np.y >= height) continue;
                    nidx = np.x + np.y * width;
                }

                int group = indexImage[nidx];
                if (group < 0 || group >= physicalGroupCount) continue;
                Ink componentType = std::get<2>(indexDict[group]);
                if (!busMergeType(componentType)) continue;

                int typeKey = static_cast<int>(setOff(componentType));
                auto inserted = firstMeshGroupByType.emplace(typeKey, group);
                if (!inserted.second) uniteGroups(inserted.first->second, group);
            }
        }

        // Compact the disjoint sets and project every physical component
        // pixel onto its final logical group before sorting and graph creation.
        std::vector<int> rootToFinal(physicalGroupCount, -1);
        std::vector<int> oldToFinal(physicalGroupCount, -1);
        std::vector<Group> mergedIndexDict;
        mergedIndexDict.reserve(physicalGroupCount);
        for (int oldGroup = 0; oldGroup < physicalGroupCount; ++oldGroup) {
            int root = findRoot(oldGroup);
            if (rootToFinal[root] < 0) {
                int finalGroup = static_cast<int>(mergedIndexDict.size());
                rootToFinal[root] = finalGroup;
                mergedIndexDict.push_back({ finalGroup,
                    std::get<1>(indexDict[root]), std::get<2>(indexDict[root]) });
            }
            oldToFinal[oldGroup] = rootToFinal[root];
        }
        for (int i = 0, lim = width * height; i < lim; ++i) {
            int oldGroup = indexImage[i];
            if (oldGroup >= 0) indexImage[i] = oldToFinal[oldGroup];
        }
        indexDict.swap(mergedIndexDict);
        writeMap.n = static_cast<int>(indexDict.size());
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

        // Keep only unique, directed signal edges after BUS groups have been
        // merged. A BUS pixel has indexImage == -1 and never enters conList.
        unordered_set<std::uint64_t> conSet;
        std::vector<std::pair<int, int>> conList;
        auto addConnection = [&](int srcGID, int dstGID) {
            if (srcGID < 0 || dstGID < 0 || srcGID == dstGID) return;
            const std::uint64_t key = (static_cast<std::uint64_t>(static_cast<std::uint32_t>(srcGID)) << 32) |
                                      static_cast<std::uint32_t>(dstGID);
            if (conSet.insert(key).second) conList.push_back({ srcGID, dstGID });
        };

        // Add connections from inks to components
        for (ivec2 p : readInks) {
            const int srcGID = indexImage[p.x + p.y * width];

            for (int k = 0; k < 4; k++) {
                ivec2 np = p + fourNeighbors[k];
                if (np.x < 0 || np.x >= width ||
                    np.y < 0 || np.y >= height) continue;

                auto ink = image[np.x + np.y * width].ink;
                if (ink == (int16_t)Ink::BundleOff || ink == (int16_t)Ink::Mesh ||
                    ink == (int16_t)Ink::Tunnel || ink == (int16_t)Ink::Bus ||
                    ink == (int16_t)Ink::ClockOff)
                    continue;

                const int dstGID = indexImage[np.x + np.y * width];
                addConnection(srcGID, dstGID);
            }
        }

        // Add connections from components to inks
        for (ivec2 p : writeInks) {
            const int dstGID = indexImage[p.x + p.y * width];

            for (int k = 0; k < 4; k++) {
                ivec2 np = p + fourNeighbors[k];
                if (np.x < 0 || np.x >= width ||
                    np.y < 0 || np.y >= height) continue;

                auto ink = image[np.x + np.y * width].ink;
                if (ink == (int16_t)Ink::BundleOff || ink == (int16_t)Ink::Mesh ||
                    ink == (int16_t)Ink::Tunnel || ink == (int16_t)Ink::Bus)
                    continue;

                const int srcGID = indexImage[np.x + np.y * width];
                addConnection(srcGID, dstGID);
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
            auto oldStateInks = stateInks;
            states = new InkState[writeMap.n];
            stateInks = new Ink[writeMap.n];
            for (int i = 0; i < writeMap.n; i++) {
                int newGID = order[transformOrder[i]];
                states[newGID] = oldStates[i];
                stateInks[newGID] = oldStateInks[i];
            }
            delete[] oldStates;
            delete[] oldStateInks;

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

        // Ordinary Bundle ink still carries bundled trace signals. Keep its
        // legacy gate/bundle edge inference separate from BUS component merges.
        auto isLogicGate4 = [](Ink in) {
            in = setOff(in);
            return in == Ink::AndOff || in == Ink::OrOff || in == Ink::XorOff ||
                   in == Ink::NotOff || in == Ink::NandOff || in == Ink::NorOff ||
                   in == Ink::XnorOff || in == Ink::BufferOff;
        };
        std::vector<int> inDegA(writeMap.n, 0);
        for (const auto& edge : conList) ++inDegA[edge.second];

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int curIdx = x + y * width;
                Ink curInk = image[curIdx].getInk();
                int curGID = indexImage[curIdx];
                if (curGID < 0 || !isLogicGate4(curInk) || inDegA[curGID] < 2) continue;

                for (int k = 0; k < 4; ++k) {
                    ivec2 np = ivec2(x, y) + fourNeighbors[k];
                    if (np.x < 0 || np.x >= width || np.y < 0 || np.y >= height) continue;
                    int nIdx = np.x + np.y * width;
                    int nGID = indexImage[nIdx];
                    if (nGID < 0 || nGID == curGID || image[nIdx].getInk() != Ink::BundleOff) continue;
                    if (inDegA[nGID] == 0) addConnection(curGID, nGID);
                }
            }
        }

        std::vector<int> inDeg(writeMap.n, 0);
        for (const auto& edge : conList) ++inDeg[edge.second];
        auto isReachable = [&](int startGID, int targetGID) {
            if (startGID == targetGID) return true;
            std::vector<unsigned char> seen(writeMap.n, 0);
            std::vector<int> queue(1, startGID);
            seen[startGID] = 1;
            for (size_t head = 0; head < queue.size(); ++head) {
                int u = queue[head];
                for (const auto& edge : conList) {
                    if (edge.first != u) continue;
                    if (edge.second == targetGID) return true;
                    if (!seen[edge.second]) {
                        seen[edge.second] = 1;
                        queue.push_back(edge.second);
                    }
                }
            }
            return false;
        };

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int curIdx = x + y * width;
                if (image[curIdx].getInk() != Ink::BundleOff) continue;
                int curGID = indexImage[curIdx];
                if (curGID < 0 || inDeg[curGID] == 0) continue;

                for (int k = 0; k < 4; ++k) {
                    ivec2 np = ivec2(x, y) + fourNeighbors[k];
                    if (np.x < 0 || np.x >= width || np.y < 0 || np.y >= height) continue;
                    int nIdx = np.x + np.y * width;
                    int nGID = indexImage[nIdx];
                    if (nGID < 0 || nGID == curGID || !isLogicGate4(image[nIdx].getInk())) continue;
                    if (inDeg[nGID] != 0 || isReachable(nGID, curGID)) continue;
                    addConnection(curGID, nGID);
                }
            }
        }

// NEWBLOCK
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

    if (openVCB::g_debug || std::getenv("VCB_DEBUG")) {
        auto inkName=[](Ink in)->const char*{
            in=setOff(in);
            switch(in){
            case Ink::TraceOff: return "TraceOff";
            case Ink::BundleOff: return "BundleOff";
            case Ink::AndOff: return "AndOff";
            case Ink::OrOff: return "OrOff";
            case Ink::XorOff: return "XorOff";
            case Ink::NotOff: return "NotOff";
            case Ink::NandOff: return "NandOff";
            case Ink::NorOff: return "NorOff";
            case Ink::XnorOff: return "XnorOff";
            case Ink::BufferOff: return "BufferOff";
            case Ink::LatchOff: return "LatchOff";
            case Ink::LedOff: return "LedOff";
            case Ink::ReadOff: return "ReadOff";
            case Ink::WriteOff: return "WriteOff";
            default: return "Other";
            }
        };
        fprintf(stderr, "\n--- DEBUG CONLIST TRACE (%zu edges) ---\n", conList.size());
        for (size_t i = 0; i < conList.size(); i++) {
            int g1 = conList[i].first;
            int g2 = conList[i].second;
            fprintf(stderr, "Edge %zu: gid %d (%s) -> gid %d (%s)\n",
                i, g1, inkName(stateInks[g1]), g2, inkName(stateInks[g2]));
        }
        fprintf(stderr, "----------------------------------------\n\n");
    }
        fprintf(stderr, "[DEBUG] Celkem nalezeno skupin (writeMap.n): %d, Spojení (conList): %zu\n", writeMap.n, conList.size());
    }
}
