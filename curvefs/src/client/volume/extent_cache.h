/*
 *  Copyright (c) 2022 NetEase Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

/*
 * Project: curve
 * Date: Monday Mar 14 19:27:02 CST 2022
 * Author: wuhanqing
 */

#ifndef CURVEFS_SRC_CLIENT_VOLUME_EXTENT_CACHE_H_
#define CURVEFS_SRC_CLIENT_VOLUME_EXTENT_CACHE_H_

#include <bvar/bvar.h>

#include <cstring>
#include <map>
#include <unordered_map>
#include <vector>

#include "curvefs/proto/metaserver.pb.h"
#include "curvefs/src/client/volume/extent.h"
#include "curvefs/src/volume/common.h"
#include "src/common/concurrent/rw_lock.h"

namespace curvefs {
namespace client {

using ::curvefs::volume::ReadPart;
using ::curvefs::volume::WritePart;

struct AllocPart {
    ExtentAllocInfo allocInfo;
    // allocate space is aligned to block size
    // but user's write are not, so we need 'padding' and 'writelength' to
    // indicate actual user's write request's on allocated space
    size_t padding = 0;
    size_t writelength = 0;
    const char* data = nullptr;
};

struct ExtentCacheOption {
    uint64_t preallocSize = 64ULL * 1024;
    uint64_t rangeSize = 1ULL * 1024 * 1024 * 1024;
    uint32_t blocksize = 4096;
};

class ExtentCache {
 public:
    ExtentCache() = default;

    static void SetOption(const ExtentCacheOption& option);

    // build extent cache from inode
    void Build(
        const google::protobuf::Map<uint64_t,
                                    curvefs::metaserver::VolumeExtentList>&
            fromInode);

    void DivideForWrite(uint64_t offset,
                        uint64_t len,
                        const char* data,
                        std::vector<WritePart>* allocated,
                        std::vector<AllocPart>* needAlloc);

    void DivideForRead(uint64_t offset,
                       uint64_t len,
                       char* data,
                       std::vector<ReadPart>* reads,
                       std::vector<ReadPart>* holes);

    void Merge(uint64_t loffset, const PExtent& pExt);

    void MarkWritten(uint64_t offset, uint64_t len);

    google::protobuf::Map<uint64_t, curvefs::metaserver::VolumeExtentList>
    ToInodePb() const;

    std::unordered_map<uint64_t, std::map<uint64_t, PExtent>>
    GetExtentsForTesting() const;

 private:
    void DivideForWriteWithinRange(const std::map<uint64_t, PExtent>& range,
                                   uint64_t offset,
                                   uint64_t len,
                                   const char* data,
                                   std::vector<WritePart>* allocated,
                                   std::vector<AllocPart>* needAlloc);

    void DivideForWriteWithinEmptyRange(uint64_t offset,
                                        uint64_t len,
                                        const char* data,
                                        std::vector<AllocPart>* needAlloc);

    void DivideForReadWithinRange(const std::map<uint64_t, PExtent>& range,
                                  uint64_t offset,
                                  uint64_t len,
                                  char* data,
                                  std::vector<ReadPart>* reads,
                                  std::vector<ReadPart>* holes);

    void MergeWithinRange(std::map<uint64_t, PExtent>* range,
                          uint64_t loffset,
                          const PExtent& extent);

    void MarkWrittenWithinRange(std::map<uint64_t, PExtent>* range,
                                uint64_t offset,
                                uint64_t len);

 private:
    mutable curve::common::RWLock lock_;

    // key is offset
    std::unordered_map<uint64_t, std::map<uint64_t, PExtent>> extents_;

 private:
    static ExtentCacheOption option_;
};

}  // namespace client
}  // namespace curvefs

#endif  // CURVEFS_SRC_CLIENT_VOLUME_EXTENT_CACHE_H_
