/*  Sirikata Transfer -- Content Distribution Network
 *  HashNameHandler.hpp
 *
 *  Copyright (c) 2009, Patrick Reiter Horn
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, class FileData;
, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SIRIKATA_HashNameHandler_HPP__
#define SIRIKATA_HashNameHandler_HPP__

#include "DownloadHandler.hpp"
#include "UploadHandler.hpp"
#include "util/Sha256.hpp"

namespace Sirikata {
namespace Transfer {

/** Stores a file-name log. of mappings from names to hashes. */
class SIRIKATA_EXPORT ComputeHashNameHandler :
        public NameLookupHandler
{
public:
    virtual void nameLookup(NameLookupHandler::TransferDataPtr *ptrRef,
            const ServiceParams &params,
            const URI &uri,
            const NameLookupHandler::Callback &cb) {
        std::string uriStr = params["requesturi"];
        if (!uriStr.empty()) {
            Fingerprint sha = SHA256::computeDigest(uriStr);
            cb(sha, uriStr, true);
        } else {
            cb(Fingerprint(), std::string(), false);
        }
    }
};

/** Stores a file-name log. of mappings from names to hashes. */
class SIRIKATA_EXPORT FilenameHashNameHandler :
        public NameLookupHandler
{
public:
    virtual void nameLookup(NameLookupHandler::TransferDataPtr *ptrRef,
            const ServiceParams &params,
            const URI &uri,
            const NameLookupHandler::Callback &cb) {
        std::string uriStr = params["requesturi"];
        if (!uriStr.empty()) {
            Fingerprint sha = SHA256::convertFromHex(URI(uriStr).filename());
            cb(sha, uriStr, true);
        } else {
            cb(Fingerprint(), std::string(), false);
        }
    }
};

}
}

#endif