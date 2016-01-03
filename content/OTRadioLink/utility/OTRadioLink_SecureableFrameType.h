/*
The OpenTRV project licenses this file to you
under the Apache Licence, Version 2.0 (the "Licence");
you may not use this file except in compliance
with the Licence. You may obtain a copy of the Licence at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the Licence is distributed on an
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
KIND, either express or implied. See the Licence for the
specific language governing permissions and limitations
under the Licence.

Author(s) / Copyright (s): Damon Hart-Davis 2015--2016
*/

/*
 * Radio message secureable frame types and related information.
 *
 * Based on 2015Q4 spec and successors:
 *     http://www.earth.org.uk/OpenTRV/stds/network/20151203-DRAFT-SecureBasicFrame.txt
 *     https://raw.githubusercontent.com/DamonHD/OpenTRV/master/standards/protocol/IoTCommsFrameFormat/SecureBasicFrame-*.txt
 */

#ifndef ARDUINO_LIB_OTRADIOLINK_SECUREABLEFRAMETYPE_H
#define ARDUINO_LIB_OTRADIOLINK_SECUREABLEFRAMETYPE_H

#include <stdint.h>

namespace OTRadioLink
    {


    // Secureable (V0p2) messages.
    //
    // Based on 2015Q4 spec and successors:
    //     http://www.earth.org.uk/OpenTRV/stds/network/20151203-DRAFT-SecureBasicFrame.txt
    //     https://raw.githubusercontent.com/DamonHD/OpenTRV/master/standards/protocol/IoTCommsFrameFormat/SecureBasicFrame-V0.1-201601.txt
    //
    // This is primarily intended for local wireless communications
    // between sensors/actuators and a local hub/concentrator,
    // but should be robust enough to traverse public WANs in some circumstances.
    //
    // This can be used in a lightweight non-secure form,
    // or in a secured form,
    // with the security nominally including authentication and encryption,
    // with algorithms and parameters agreed in advance between leaf and hub,
    // and possibly varying by message type.
    // The initial supported auth/enc crypto mechanism (as of 2015Q4)
    // is AES-GCM with 128-bit pre-shared keys (and pre-shared IDs).
    //
    // The leading byte received indicates the length of frame that follows,
    // with the following byte indicating the frame type.
    // The leading frame-length byte allows efficient packet RX with many low-end radios.
    //
    // Frame types of 32/0x20 or above are reserved to OpenTRV to define.
    // Frame types < 32/0x20 (ignoring secure bit) are defined as
    // local-use-only and may be defined and used privately
    // (within a local radio network ~100m max or local wired network)
    // for any reasonable purpose providing use is generally consistent with
    // the rest of the protocol,
    // and providing that frames are not allowed to escape the local network.
    enum FrameType_Secureable
        {
        // No message should be type 0x00/0x01 (nor 0x7f/0xff).
        FTS_NONE                        = 0,
        FTS_INVALID_HIGH                = 0x7f,

        // Frame types < 32/0x20 (ignoring secure bit) are defined as local-use-only.
        FTS_MAX_LOCAL_TYPE              = 31,
        // Frame types of 32/0x20 or above are reserved to OpenTRV to define.
        FTS_MAN_PUBLIC_TYPE             = 32,

        // "I'm alive" message with empty (zero-length) message body.
        // Same crypto algorithm as 'O' frame type to be used when secure.
        // This message can be sent asynchronously,
        // or after a short randomised delay in response to a broadcast liveness query.
        // ID should not be zero length as this makes little sense anonymously.
        FTS_ALIVE                       = '!',

        // OpenTRV basic valve/sensor leaf-to-hub frame (secure if high-bit set).
        FTS_BasicSensorOrValve          = 'O', // 0x4f
        };

    // A high bit set (0x80) in the type indicates the secure message format variant.
    // The frame type is part of the authenticated data.
    const static uint8_t SECUREABLE_FRAME_TYPE_SEC_FLAG = 0x80;

    // Logical header for the secureable frame format.
    // Intended to be efficient to hold and work with in memory
    // and to convert to and from wire format.
    // All of this header should be (in wire format) authenticated for secure frames.
    // Note: fl = hl-1 + bl + tl = 3+il + bl + tl
    struct SecurableFrameHeader
        {
        // Create an instance as an invalid frame header (invalid length and ID).
        SecurableFrameHeader() : fl(0) { id[0] = 0xff; }

        // Returns true if the frame header in this struct instance is invalid.
        // This is only reliable if all manipulation of struct content
        // is by the member functions.
        bool isInvalid() { return(0 == fl); }

        // Maximum (small) frame size is 64, excluding fl byte.
        static const uint8_t maxSmallFrameSize = 63;
        // Frame length excluding/after this byte; zero indicates an invalid frame.
        // Appears first on the wire to support radio hardware packet handling.
        //     fl = hl-1 + bl + tl = 3+il + bl + tl
        // where hl header length, bl body length, tl trailer length
        // Should usually be set last to leave header clearly invalid until complete.
        uint8_t fl;

        // Frame type nominally from FrameType_Secureable (bits 0-6, [1,126]).
        // Top bit indicates secure frame if 1/true.
        uint8_t fType;
        bool isSecure() const { return(0 != (0x80 & fType)); }

        // Frame sequence number mod 16 [0,15] (bits 4 to 7) and ID length [0,15] (bits 0-3).
        //
        // Sequence number increments from 0, wraps at 15;
        // increment is skipped for repeat TXes used for noise immunity.
        // If a counter is used as part of (eg) security IV/nonce
        // then these 4 bits may be its least significant bits.
        uint8_t seqIl;
        // Get frame sequence number mod 16 [0,15].
        uint8_t getSeq() const { return((seqIl >> 4) & 0xf); }
        // Get il (ID length) [0,15].
        uint8_t getIl() const { return(seqIl & 0xf); }

        // ID bytes (0 implies anonymous, 1 or 2 typical domestic, length il).
        //
        // This is the first il bytes of the leaf's (64-bit) full ID.
        // Thus this is typically the ID of the sending sensor/valve/etc,
        // but may under some circumstances (depending on message type)
        // be the ID of the target/recipient.
        //
        // Initial and 'small frame' implementations are limited to 8 bytes of ID
        const static uint8_t maxIDLength = 8;
        uint8_t id[maxIDLength];

        // Body length including any padding [0,251] but generally << 60.
        uint8_t bl;
        // Compute the offset of the body from the start of the frame after nominal fl (ie where the fType offset is zero).
        uint8_t getBodyOffset() const { return(3 + getIl()); }

        // Compute tl (trailer length) [1,251]; must == 1 for insecure frame.
        // Other fields must be valid for this to return a valid answer.
        uint8_t getTl() const { return(fl - 3 - getIl() - bl); }
        // Compute the offset of the trailer from the start of the frame after nominal fl (ie where the fType offset is zero).
        uint8_t getTrailerOffset() const { return(3 + getIl() + bl); }


        // Check parameters for, and if valid then encode into the given buffer, the header for a small secureable frame.
        // Parameters:
        //  * buf  buffer to encode header to, of at least length buflen; never NULL
        //  * buflen  available length in buf; if too small for encoded header routine will fail (return 0)
        //  * secure_ true if this is to be a secure frame
        //  * fType_  frame type (without secure bit) in range ]FTS_NONE,FTS_INVALID_HIGH[ ie exclusive
        //  * seqNum_  least-significant 4 bits are 4 lsbs of frame sequence number
        //  * il_  ID length in bytes at most 8 (could be 15 for non-small frames)
        //  * id_  source of ID bytes, at least il_ long; NULL means pre-filled but must not start with 0xff.
        //  * bl_  body length in bytes [0,251] at most
        //  * tl_  trailer length [1,251[ at most, always == 1 for non-secure frame
        //
        // This does not permit encoding of frames with more than 64 bytes (ie 'small' frames only).
        // This does not deal with encoding the body or the trailer.
        // Having validated the parameters they are copied into the structure
        // and then into the supplied buffer, returning the number of bytes written.
        //
        // Performs at least the 'Quick Integrity Checks' from the spec, eg SecureBasicFrame-V0.1-201601.txt
        //  1) fl >= 4 (type, seq/il, bl, trailer bytes)
        //  2) fl may be further constrained by system limits, typically to <= 64
        //  3) type (the first frame byte) is never 0x00, 0x80, 0x7f, 0xff.
        //  4) il <= 8 for initial implementations (internal node ID is 8 bytes)
        //  5) il <= fl - 4 (ID length; minimum of 4 bytes of other overhead)
        //  6) bl <= fl - 4 - il (body length; minimum of 4 bytes of other overhead)
        //  7) the final frame byte (the final trailer byte) is never 0x00 nor 0xff
        //  8) tl == 1 for non-secure, tl >= 1 for secure (tl = fl - 3 - il - bl)        // Note: fl = hl-1 + bl + tl = 3+il + bl + tl
        //
        // (If the parameters are invalid or the buffer too small, 0 is returned to indicate an error.)
        // The fl byte in the structure is set to the frame length, else 0 in case of any error.
        // Returns number of bytes of encoded header excluding nominally-leading fl length byte; 0 in case of error.
        uint8_t checkAndEncodeSmallFrameHeader(uint8_t *buf, uint8_t buflen,
                                               bool secure_, FrameType_Secureable fType_,
                                               uint8_t seqNum_,
                                               uint8_t il_, const uint8_t *id_,
                                               uint8_t bl_,
                                               uint8_t tl_);

        // Loads the node ID from the EEPROM or other non-volatile ID store.
        // Can be called before a call to checkAndEncodeSmallFrameHeader() with id_ == NULL.
        // Pads at end with 0xff if the EEPROM ID is shorter than the maximum 'short frame' ID.
        uint8_t loadIDFromEEPROM();

        // Decode header and check parameters/validity for inbound short secureable frame.
        //  * buf  buffer to decode header from, of at least length buflen; never NULL
        //  * buflen  available length in buf; if too small for encoded header routine will fail (return 0)
        //
        // Performs at least the 'Quick Integrity Checks' from the spec, eg SecureBasicFrame-V0.1-201601.txt
        //  1) fl >= 4 (type, seq/il, bl, trailer bytes)
        //  2) fl may be further constrained by system limits, typically to <= 64
        //  3) type (the first frame byte) is never 0x00, 0x80, 0x7f, 0xff.
        //  4) il <= 8 for initial implementations (internal node ID is 8 bytes)
        //  5) il <= fl - 4 (ID length; minimum of 4 bytes of other overhead)
        //  6) bl <= fl - 4 - il (body length; minimum of 4 bytes of other overhead)
        //  7) the final frame byte (the final trailer byte) is never 0x00 nor 0xff
        //  8) tl == 1 for non-secure, tl >= 1 for secure (tl = fl - 3 - il - bl)        // Note: fl = hl-1 + bl + tl = 3+il + bl + tl
        //
        // (If the header is invalid or the buffer too small, 0 is returned to indicate an error.)
        // The fl byte in the structure is set to the frame length, else 0 in case of any error.
        // Returns number of bytes of decoded header excluding nominally-leading fl length byte; 0 in case of error.
        uint8_t checkAndDecodeSmaleFrameHeader(const uint8_t *buf, uint8_t buflen);
        };


    }

#endif
