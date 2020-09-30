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

Author(s) / Copyright (s): Damon Hart-Davis 2016
                           Deniz Erbilgin 2017
*/

/*
 * OTRadValve tests of secure frames dependent on OTASEGCM
 */

// Only enable these tests if the OTAESGCM library is marked as available.
#if defined(EXT_AVAILABLE_ARDUINO_LIB_OTAESGCM)


#include <gtest/gtest.h>
#include <stdio.h>
#include <stdlib.h>

#include <OTAESGCM.h>
#include <OTRadioLink.h>


static const int AES_KEY_SIZE = 128; // in bits
static const int GCM_NONCE_LENGTH = 12; // in bytes
static const int GCM_TAG_LENGTH = 16; // in bytes (default 16, 12 possible)

// All-zeros const 16-byte/128-bit key.
// Can be used for other purposes.
static const uint8_t zeroBlock[16] = { };

// Max stack usage in bytes
// 20170511
//           enc, dec, enc*, dec*
// - DE:     208, 208, 208,  208
// - DHD:    ???, ???, 358,  ???
// - Travis: 192, 224, ???,  ???
// * using a workspace
#ifndef __APPLE__
static constexpr unsigned int maxStackSecureFrameEncode = 400;  // 328 for gcc. Clang uses more stack
static constexpr unsigned int maxStackSecureFrameDecode = 400;  //
#else
// On DHD's system, secure frame enc/decode uses 358 bytes (20170511)
static constexpr unsigned int maxStackSecureFrameEncode = 416;
static constexpr unsigned int maxStackSecureFrameDecode = 416;
#endif // __APPLE__

TEST(SimpleSecureFrame, StackCheckerWorks)
{
    // Set up stack usage checks
    OTV0P2BASE::RAMEND = OTV0P2BASE::getSP();
    OTV0P2BASE::MemoryChecks::resetMinSP();
    OTV0P2BASE::MemoryChecks::recordIfMinSP();
    const size_t baseStack = OTV0P2BASE::MemoryChecks::getMinSP();
    // Uncomment to print stack usage
    EXPECT_NE((size_t)0, baseStack);
}

TEST(SimpleSecureFrame, NullCompilation)
{
    OTRadioLink::SimpleSecureFrame32or0BodyTXV0p2Null &sf = OTRadioLink::SimpleSecureFrame32or0BodyTXV0p2Null::getInstance();
    EXPECT_NE(nullptr, &sf);
}


// Test quick integrity checks, for TX and RX.
//
// DHD20161107: imported from test_SECFRAME.ino testFramQIC().
TEST(OTAESGCMSecureFrame, FrameQIC)
{
    OTRadioLink::SecurableFrameHeader sfh;
    uint8_t _id[OTRadioLink::SecurableFrameHeader::maxIDLength + 1];  // Make buffer large enough for fail case.
    OTRadioLink::OTBuf_t id(_id, OTRadioLink::SecurableFrameHeader::maxIDLength);
    OTRadioLink::OTBuf_t minimalID(_id, 1);
    OTRadioLink::OTBuf_t id2bytes(_id, 2);
    OTRadioLink::OTBuf_t largeID(_id, sizeof(_id));  // Make buffer large enough for fail case.
    uint8_t _buf[OTRadioLink::SecurableFrameHeader::maxSmallFrameSize + 1];
    OTRadioLink::OTBuf_t buf(_buf, sizeof(_buf));

    OTRadioLink::OTBuf_t nullbuf(nullptr, 0);

    // Uninitialised SecurableFrameHeader should be 'invalid'.
    EXPECT_TRUE(sfh.isInvalid());
    // ENCODE
    // Test various bad input combos that should be caught by QIC.
    // Can futz (some of the) inputs that should not matter...
    // Should fail with bad ID length.
    EXPECT_EQ(0, sfh.encodeHeader(
                        buf,
                        false, OTRadioLink::FTS_BasicSensorOrValve,
                        OTV0P2BASE::randRNG8(),
                        largeID.buf,
                        largeID.bufsize,
                        2,
                        1));
    // Should fail with bad buffer length.
    EXPECT_EQ(0, sfh.encodeHeader(
                        nullbuf,
                        false, OTRadioLink::FTS_BasicSensorOrValve,
                        OTV0P2BASE::randRNG8(),
                        id2bytes.buf,
                        id2bytes.bufsize,
                        2,
                        1));
    // Should fail with bad frame type.
    EXPECT_EQ(0, sfh.encodeHeader(
                        buf,
                        OTV0P2BASE::randRNG8NextBoolean(), OTRadioLink::FTS_NONE,
                        OTV0P2BASE::randRNG8(),
                        id2bytes.buf,
                        id2bytes.bufsize,
                        2,
                        1));
    EXPECT_EQ(0, sfh.encodeHeader(
                        buf,
                        OTV0P2BASE::randRNG8NextBoolean(), OTRadioLink::FTS_INVALID_HIGH,
                        OTV0P2BASE::randRNG8(),
                        id2bytes.buf,
                        id2bytes.bufsize,
                        2,
                        1));
    // Should fail with impossible body length.
    EXPECT_EQ(0, sfh.encodeHeader(
                        buf,
                        OTV0P2BASE::randRNG8NextBoolean(), OTRadioLink::FTS_ALIVE,
                        OTV0P2BASE::randRNG8(),
                        minimalID.buf,
                        minimalID.bufsize,
                        252,
                        1));
    // Should fail with impossible trailer length.
    EXPECT_EQ(0, sfh.encodeHeader(
                        buf,
                        OTV0P2BASE::randRNG8NextBoolean(), OTRadioLink::FTS_ALIVE,
                        OTV0P2BASE::randRNG8(),
                        minimalID.buf,
                        minimalID.bufsize,
                        0,
                        0));
    EXPECT_EQ(0, sfh.encodeHeader(
                        buf,
                        OTV0P2BASE::randRNG8NextBoolean(), OTRadioLink::FTS_ALIVE,
                        OTV0P2BASE::randRNG8(),
                        minimalID.buf,
                        minimalID.bufsize,
                        0,
                        252));
    // Should fail with impossible body + trailer length (for small frame).
    EXPECT_EQ(0, sfh.encodeHeader(
                        buf,
                        OTV0P2BASE::randRNG8NextBoolean(), OTRadioLink::FTS_ALIVE,
                        OTV0P2BASE::randRNG8(),
                        minimalID.buf,
                        minimalID.bufsize,
                        32,
                        32));
    // "I'm Alive!" message with 1-byte ID should succeed and be of full header length (5).
    EXPECT_EQ(5, sfh.encodeHeader(
                        buf,
                        false, OTRadioLink::FTS_ALIVE,
                        OTV0P2BASE::randRNG8(),
                        minimalID.buf,  // Minimal (non-empty) ID.
                        minimalID.bufsize,
                        0, // No payload.
                        1));
    // Large but legal body size.
    EXPECT_EQ(5, sfh.encodeHeader(
                        buf,
                        false, OTRadioLink::FTS_ALIVE,
                        OTV0P2BASE::randRNG8(),
                        minimalID.buf,  // Minimal (non-empty) ID.
                        minimalID.bufsize,
                        32,
                        1));
    // DECODE  // TODO SWITCH THIS!
    // Test various bad input combos that should be caught by QIC.
    // Can futz (some of the) inputs that should not matter...
    // Should fail with bad (too small) buffer.
    buf.buf[0] = OTV0P2BASE::randRNG8();
    EXPECT_EQ(0, sfh.decodeHeader(nullbuf));
    // Should fail with bad (too small) frame length.
    buf.buf[0] = 3 & OTV0P2BASE::randRNG8();
    EXPECT_EQ(0, sfh.decodeHeader(buf));
    // Should fail with bad (too large) frame length for 'small' frame.
    buf.buf[0] = 64;
    EXPECT_EQ(0, sfh.decodeHeader(buf));
    // Should fail with bad (too large) frame header for the input buffer.
    uint8_t _buf1[] = { 0x08, 0x4f, 0x02, 0x80, 0x81 }; // , 0x02, 0x00, 0x01, 0x23 };
    OTRadioLink::OTBuf_t buf1(_buf1, sizeof(_buf1));
    EXPECT_EQ(0, sfh.decodeHeader(buf1));
    // Should fail with bad trailer byte (illegal 0x00 value).
    uint8_t _buf2[] = { 0x08, 0x4f, 0x02, 0x80, 0x81, 0x02, 0x00, 0x01, 0x00 };
    OTRadioLink::OTBuf_t buf2(_buf2, sizeof(_buf2));
    EXPECT_EQ(0, sfh.decodeHeader(buf2));
    // Should fail with bad trailer byte (illegal 0xff value).
    uint8_t _buf3[] = { 0x08, 0x4f, 0x02, 0x80, 0x81, 0x02, 0x00, 0x01, 0xff };
    OTRadioLink::OTBuf_t buf3(_buf3, sizeof(_buf3));
    EXPECT_EQ(0, sfh.decodeHeader(buf3));
    // TODO
}

// Test encoding of header for TX.
//
// DHD20161107: imported from test_SECFRAME.ino testFrameHeaderEncoding().
TEST(OTAESGCMSecureFrame, FrameHeaderEncoding)
{

    OTRadioLink::SecurableFrameHeader sfh;
    uint8_t _id[OTRadioLink::SecurableFrameHeader::maxIDLength];  // Make buffer large enough for fail case.
    OTRadioLink::OTBuf_t id2bytes(_id, 2);
    uint8_t _buf[OTRadioLink::SecurableFrameHeader::maxSmallFrameSize];
    OTRadioLink::OTBuf_t buf(_buf, sizeof(_buf));

    //
    // Test vector 1 / example from the spec.
    //Example insecure frame, valve unit 0% open, no call for heat/flags/stats.
    //In this case the frame sequence number is zero, and ID is 0x80 0x81.
    //
    //08 4f 02 80 81 02 | 00 01 | 23
    //
    //08 length of header (8) after length byte 5 + body 2 + trailer 1
    //4f 'O' insecure OpenTRV basic frame
    //02 0 sequence number, ID length 2
    //80 ID byte 1
    //81 ID byte 2
    //02 body length 2
    //00 valve 0%, no call for heat
    //01 no flags or stats, unreported occupancy
    //23 CRC value
    id2bytes.buf[0] = 0x80;
    id2bytes.buf[1] = 0x81;
    EXPECT_EQ(6, sfh.encodeHeader(
                        buf,
                        false, OTRadioLink::FTS_BasicSensorOrValve,
                        0,
                        id2bytes.buf,
                        id2bytes.bufsize,
                        2,
                        1));
    EXPECT_EQ(0x08, buf.buf[0]);
    EXPECT_EQ(0x4f, buf.buf[1]);
    EXPECT_EQ(0x02, buf.buf[2]);
    EXPECT_EQ(0x80, buf.buf[3]);
    EXPECT_EQ(0x81, buf.buf[4]);
    EXPECT_EQ(0x02, buf.buf[5]);
    // Check related parameters.
    EXPECT_EQ(8, sfh.fl);
    EXPECT_EQ(6, sfh.getBodyOffset());
    EXPECT_EQ(8, sfh.getTrailerOffset());
    //
    // Test vector 2 / example from the spec.
    //Example insecure frame, no valve, representative minimum stats {"b":1}
    //In this case the frame sequence number is zero, and ID is 0x80 0x81.
    //
    //0e 4f 02 80 81 08 | 7f 11 7b 22 62 22 3a 31 | 61
    //
    //0e length of header (14) after length byte 5 + body 8 + trailer 1
    //4f 'O' insecure OpenTRV basic frame
    //02 0 sequence number, ID length 2
    //80 ID byte 1
    //81 ID byte 2
    //08 body length 8
    //7f no valve, no call for heat
    //11 stats present flag only, unreported occupancy
    //7b 22 62 22 3a 31  {"b":1  Stats: note that implicit trailing '}' is not sent.
    //61 CRC value
    id2bytes.buf[0] = 0x80;
    id2bytes.buf[1] = 0x81;
    EXPECT_EQ(6, sfh.encodeHeader(buf,
                        false, OTRadioLink::FTS_BasicSensorOrValve,
                        0,
                        id2bytes.buf,
                        id2bytes.bufsize,
                        8,
                        1));
    EXPECT_EQ(0x0e, buf.buf[0]);
    EXPECT_EQ(0x4f, buf.buf[1]);
    EXPECT_EQ(0x02, buf.buf[2]);
    EXPECT_EQ(0x80, buf.buf[3]);
    EXPECT_EQ(0x81, buf.buf[4]);
    EXPECT_EQ(0x08, buf.buf[5]);
    // Check related parameters.
    EXPECT_EQ(14, sfh.fl);
    EXPECT_EQ(6, sfh.getBodyOffset());
    EXPECT_EQ(14, sfh.getTrailerOffset());
}

// Test decoding of header for RX.
//
// DHD20161107: imported from test_SECFRAME.ino testFrameHeaderDecoding().
TEST(OTAESGCMSecureFrame, FrameHeaderDecoding)
{
    OTRadioLink::SecurableFrameHeader sfh;
    //
    // Test vector 1 / example from the spec.
    //Example insecure frame, valve unit 0% open, no call for heat/flags/stats.
    //In this case the frame sequence number is zero, and ID is 0x80 0x81.
    //
    //08 4f 02 80 81 02 | 00 01 | 23
    //
    //08 length of header (8) after length byte 5 + body 2 + trailer 1
    //4f 'O' insecure OpenTRV basic frame
    //02 0 sequence number, ID length 2
    //80 ID byte 1
    //81 ID byte 2
    //02 body length 2
    //00 valve 0%, no call for heat
    //01 no flags or stats, unreported occupancy
    //23 CRC value
    const uint8_t buf1[] = { 0x08, 0x4f, 0x02, 0x80, 0x81, 0x02, 0x00, 0x01, 0x23 };
    EXPECT_EQ(6, sfh.decodeHeader(buf1, sizeof(buf1)));
    // Check decoded parameters.
    EXPECT_EQ(8, sfh.fl);
    EXPECT_EQ(2, sfh.getIl());
    EXPECT_EQ(0x80, sfh.id[0]);
    EXPECT_EQ(0x81, sfh.id[1]);
    EXPECT_EQ(2, sfh.bl);
    EXPECT_EQ(1, sfh.getTl());
    EXPECT_EQ(6, sfh.getBodyOffset());
    EXPECT_EQ(8, sfh.getTrailerOffset());
    //
    // Test vector 2 / example from the spec.
    //Example insecure frame, no valve, representative minimum stats {"b":1}
    //In this case the frame sequence number is zero, and ID is 0x80 0x81.
    //
    //0e 4f 02 80 81 08 | 7f 11 7b 22 62 22 3a 31 | 61
    //
    //0e length of header (14) after length byte 5 + body 8 + trailer 1
    //4f 'O' insecure OpenTRV basic frame
    //02 0 sequence number, ID length 2
    //80 ID byte 1
    //81 ID byte 2
    //08 body length 8
    //7f no valve, no call for heat
    //11 stats present flag only, unreported occupancy
    //7b 22 62 22 3a 31  {"b":1  Stats: note that implicit trailing '}' is not sent.
    //61 CRC value
    static const uint8_t buf2[] = { 0x0e, 0x4f, 0x02, 0x80, 0x81, 0x08, 0x7f, 0x11, 0x7b, 0x22, 0x62, 0x22, 0x3a, 0x31, 0x61 };
    EXPECT_EQ(6, sfh.decodeHeader(buf2, sizeof(buf2)));
    // Check decoded parameters.
    EXPECT_EQ(14, sfh.fl);
    EXPECT_EQ(2, sfh.getIl());
    EXPECT_EQ(0x80, sfh.id[0]);
    EXPECT_EQ(0x81, sfh.id[1]);
    EXPECT_EQ(8, sfh.bl);
    EXPECT_EQ(1, sfh.getTl());
    EXPECT_EQ(6, sfh.getBodyOffset());
    EXPECT_EQ(14, sfh.getTrailerOffset());
}

// Test CRC computation for insecure frames.
//
// DHD20161107: imported from test_SECFRAME.ino testNonsecureFrameCRC().
TEST(OTAESGCMSecureFrame, NonsecureFrameCRC)
{
    //
    // Test vector 1 / example from the spec.
    //Example insecure frame, valve unit 0% open, no call for heat/flags/stats.
    //In this case the frame sequence number is zero, and ID is 0x80 0x81.
    //
    //08 4f 02 80 81 02 | 00 01 | 23
    //
    //08 length of header (8) after length byte 5 + body 2 + trailer 1
    //4f 'O' insecure OpenTRV basic frame
    //02 0 sequence number, ID length 2
    //80 ID byte 1
    //81 ID byte 2
    //02 body length 2
    //00 valve 0%, no call for heat
    //01 no flags or stats, unreported occupancy
    //23 CRC value
    const uint8_t buf1[] = { 0x08, 0x4f, 0x02, 0x80, 0x81, 0x02, 0x00, 0x01, 0x23 };
    OTRadioLink::OTDecodeData_T fd1(buf1, nullptr);
    EXPECT_EQ(6, fd1.sfh.decodeHeader(buf1, 6));
    EXPECT_EQ(0x23, fd1.sfh.computeNonSecureCRC(buf1, sizeof(buf1) - 1));
    // Decode entire frame, emulating RX, structurally validating the header then checking the CRC.
    EXPECT_TRUE(0 != fd1.sfh.decodeHeader(buf1, sizeof(buf1)));
    EXPECT_TRUE(0 != decodeNonsecure(fd1));
    //
    // Test vector 2 / example from the spec.
    //Example insecure frame, no valve, representative minimum stats {"b":1}
    //In this case the frame sequence number is zero, and ID is 0x80 0x81.
    //
    //0e 4f 02 80 81 08 | 7f 11 7b 22 62 22 3a 31 | 61
    //
    //0e length of header (14) after length byte 5 + body 8 + trailer 1
    //4f 'O' insecure OpenTRV basic frame
    //02 0 sequence number, ID length 2
    //80 ID byte 1
    //81 ID byte 2
    //08 body length 8
    //7f no valve, no call for heat
    //11 stats present flag only, unreported occupancy
    //7b 22 62 22 3a 31  {"b":1  Stats: note that implicit trailing '}' is not sent.
    //61 CRC value
    const uint8_t buf2[] = { 0x0e, 0x4f, 0x02, 0x80, 0x81, 0x08, 0x7f, 0x11, 0x7b, 0x22, 0x62, 0x22, 0x3a, 0x31, 0x61 };
    OTRadioLink::OTDecodeData_T fd2(buf2, nullptr);
    // Just decode and check the frame header first
    EXPECT_EQ(6, fd2.sfh.decodeHeader(buf2, 6));
    EXPECT_EQ(0x61, fd2.sfh.computeNonSecureCRC(buf2, sizeof(buf2) - 1));
    // Decode entire frame, emulating RX, structurally validating the header then checking the CRC.
    EXPECT_TRUE(0 != fd2.sfh.decodeHeader(buf2, sizeof(buf2)));
    EXPECT_TRUE(0 != decodeNonsecure(fd2));
}

// Test encoding of entire non-secure frame for TX.
//
// DHD20161107: imported from test_SECFRAME.ino testNonsecureSmallFrameEncoding().
TEST(OTAESGCMSecureFrame, NonsecureSmallFrameEncoding)
{
    uint8_t _id[OTRadioLink::SecurableFrameHeader::maxIDLength];  // Make buffer large enough for fail case.
    OTRadioLink::OTBuf_t id2bytes(_id, 2);
    uint8_t buf[OTRadioLink::SecurableFrameHeader::maxSmallFrameSize];
    uint8_t body[2] = { 0x00, 0x01 };

    OTRadioLink::OTEncodeData_T fd(body, sizeof(body), buf, sizeof(buf));
    fd.fType = OTRadioLink::FTS_BasicSensorOrValve;

    //
    // Test vector 1 / example from the spec.
    //Example insecure frame, valve unit 0% open, no call for heat/flags/stats.
    //In this case the frame sequence number is zero, and ID is 0x80 0x81.
    //
    //08 4f 02 80 81 02 | 00 01 | 23
    //
    //08 length of header (8) after length byte 5 + body 2 + trailer 1
    //4f 'O' insecure OpenTRV basic frame
    //02 0 sequence number, ID length 2
    //80 ID byte 1
    //81 ID byte 2
    //02 body length 2
    //00 valve 0%, no call for heat
    //01 no flags or stats, unreported occupancy
    //23 CRC value
    id2bytes.buf[0] = 0x80;
    id2bytes.buf[1] = 0x81;
    EXPECT_EQ(9, OTRadioLink::encodeNonsecure(
                                    fd,
                                    0,
                                    id2bytes.buf,
                                    id2bytes.bufsize));
    EXPECT_EQ(0x08, buf[0]);
    EXPECT_EQ(0x4f, buf[1]);
    EXPECT_EQ(0x02, buf[2]);
    EXPECT_EQ(0x80, buf[3]);
    EXPECT_EQ(0x81, buf[4]);
    EXPECT_EQ(0x02, buf[5]);
    EXPECT_EQ(0x00, buf[6]);
    EXPECT_EQ(0x01, buf[7]);
    EXPECT_EQ(0x23, buf[8]);
}

// Test simple plain-text padding for encryption.
//
// DHD20161107: imported from test_SECFRAME.ino testSimplePadding().
TEST(OTAESGCMSecureFrame, SimplePadding)
{
    uint8_t buf[OTRadioLink::ENC_BODY_SMALL_FIXED_CTEXT_SIZE];
    // Provoke failure with NULL buffer.
    EXPECT_EQ(0, OTRadioLink::SimpleSecureFrame32or0BodyTXBase::pad32BBuffer(NULL, 0x1f & OTV0P2BASE::randRNG8()));
    // Provoke failure with over-long unpadded plain-text.
    EXPECT_EQ(0, OTRadioLink::SimpleSecureFrame32or0BodyTXBase::pad32BBuffer(buf, 1 + OTRadioLink::ENC_BODY_SMALL_FIXED_PTEXT_MAX_SIZE));
    // Check padding in case with single random data byte (and the rest of the buffer set differently).
    // Check the entire padded result for correctness.
    const uint8_t db0 = OTV0P2BASE::randRNG8();
    buf[0] = db0;
    memset(buf+1, ~db0, sizeof(buf)-1);
    EXPECT_EQ(32, OTRadioLink::SimpleSecureFrame32or0BodyTXBase::pad32BBuffer(buf, 1));
    EXPECT_EQ(db0, buf[0]);
    for(int i = 30; --i > 0; ) { EXPECT_EQ(0, buf[i]); }
    EXPECT_EQ(30, buf[31]);
    // Ensure that unpadding works.
    EXPECT_EQ(1, OTRadioLink::SimpleSecureFrame32or0BodyRXBase::unpad32BBuffer(buf));
    EXPECT_EQ(db0, buf[0]);
}

// Test simple fixed-size NULL enc/dec behaviour.
//
// DHD20161107: imported from test_SECFRAME.ino testSimpleNULLEncDec().
TEST(OTAESGCMSecureFrame, SimpleNULLEncDec)
{
    OTRadioLink::SimpleSecureFrame32or0BodyTXBase::fixed32BTextSize12BNonce16BTagSimpleEnc_fn_t &e = OTRadioLink::fixed32BTextSize12BNonce16BTagSimpleEnc_NULL_IMPL;
    OTRadioLink::SimpleSecureFrame32or0BodyRXBase::fixed32BTextSize12BNonce16BTagSimpleDec_fn_t &d = OTRadioLink::fixed32BTextSize12BNonce16BTagSimpleDec_NULL_IMPL;
    // Check that calling the NULL enc routine with bad args fails.
    EXPECT_TRUE(!e(NULL, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL));
    static const uint8_t plaintext1[32] = { 'a', 'b', 'c', 'd', 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4 };
    static const uint8_t nonce1[12] = { 'q', 'u', 'i', 'c', 'k', ' ', 6, 5, 4, 3, 2, 1 };
    static const uint8_t authtext1[2] = { 'H', 'i' };
    // Output ciphertext and tag buffers.
    uint8_t workspace[1];
    uint8_t co1[32], to1[16];
    EXPECT_TRUE(e(workspace, sizeof(workspace), zeroBlock, nonce1, authtext1, sizeof(authtext1), plaintext1, co1, to1));
    EXPECT_EQ(0, memcmp(plaintext1, co1, 32));
    EXPECT_EQ(0, memcmp(nonce1, to1, 12));
    EXPECT_EQ(0, to1[12]);
    EXPECT_EQ(0, to1[15]);
    // Check that calling the NULL decc routine with bad args fails.
    EXPECT_TRUE(!d(NULL, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL));
    // Decode the ciphertext and tag from above and ensure that it 'works'.
    uint8_t plaintext1Decoded[32];
    EXPECT_TRUE(d(workspace, sizeof(workspace), zeroBlock, nonce1, authtext1, sizeof(authtext1), co1, to1, plaintext1Decoded));
    EXPECT_EQ(0, memcmp(plaintext1, plaintext1Decoded, 32));
}

// Test a simple fixed-size enc/dec function pair.
// Aborts with Assert...() in case of failure.
static void runSimpleEncDec(OTRadioLink::SimpleSecureFrame32or0BodyTXBase::fixed32BTextSize12BNonce16BTagSimpleEnc_fn_t &e,
                            OTRadioLink::SimpleSecureFrame32or0BodyRXBase::fixed32BTextSize12BNonce16BTagSimpleDec_fn_t &d)
{
    // Check that calling the NULL enc routine with bad args fails.
    EXPECT_TRUE(!e(NULL, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL));
    // Try with plaintext and authext...
    static const uint8_t plaintext1[32] = { 'a', 'b', 'c', 'd', 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4 };
    static const uint8_t nonce1[12] = { 'q', 'u', 'i', 'c', 'k', ' ', 6, 5, 4, 3, 2, 1 };
    static const uint8_t authtext1[2] = { 'H', 'i' };
    // Output ciphertext and tag buffers.
    // Create a workspace big enough for any operation.
    uint8_t workspace[OTAESGCM::OTAES128GCMGenericWithWorkspace<>::workspaceRequired];
    uint8_t co1[32], to1[16];
    EXPECT_TRUE(e(workspace, sizeof(workspace), zeroBlock, nonce1, authtext1, sizeof(authtext1), plaintext1, co1, to1));
    // Check that calling the NULL dec routine with bad args fails.
    EXPECT_TRUE(!d(NULL, 0, NULL, NULL, NULL, 0, NULL, NULL, NULL));
    // Decode the ciphertext and tag from above and ensure that it 'works'.
    uint8_t plaintext1Decoded[32];
    EXPECT_TRUE(d(workspace, sizeof(workspace), zeroBlock, nonce1, authtext1, sizeof(authtext1), co1, to1, plaintext1Decoded));
    EXPECT_EQ(0, memcmp(plaintext1, plaintext1Decoded, 32));
    // Try with authtext and no plaintext.
    EXPECT_TRUE(e(workspace, sizeof(workspace), zeroBlock, nonce1, authtext1, sizeof(authtext1), NULL, co1, to1));
    EXPECT_TRUE(d(workspace, sizeof(workspace), zeroBlock, nonce1, authtext1, sizeof(authtext1), NULL, to1, plaintext1Decoded));
}

// Test basic access to crypto features.
// Check basic operation of the simple fixed-sized encode/decode routines.
//
// DHD20161107: imported from test_SECFRAME.ino testCryptoAccess().
TEST(OTAESGCMSecureFrame, CryptoAccess)
{
    // NULL enc/dec.
    runSimpleEncDec(OTRadioLink::fixed32BTextSize12BNonce16BTagSimpleEnc_NULL_IMPL,
                  OTRadioLink::fixed32BTextSize12BNonce16BTagSimpleDec_NULL_IMPL);
    // AES-GCM 128-bit key enc/dec.
    runSimpleEncDec(OTAESGCM::fixed32BTextSize12BNonce16BTagSimpleEnc_DEFAULT_WITH_LWORKSPACE,
                  OTAESGCM::fixed32BTextSize12BNonce16BTagSimpleDec_DEFAULT_WITH_LWORKSPACE);
}

// Check WITH_WORKSPACE methods using NIST GCMVS test vector.
// Test via fixed32BTextSize12BNonce16BTagSimpleEnc_DEFAULT_STATELESS interface.
// See http://csrc.nist.gov/groups/STM/cavp/documents/mac/gcmvs.pdf
// See http://csrc.nist.gov/groups/STM/cavp/documents/mac/gcmtestvectors.zip
//
//[Keylen = 128]
//[IVlen = 96]
//[PTlen = 256]
//[AADlen = 128]
//[Taglen = 128]
//
//Count = 0
//Key = 298efa1ccf29cf62ae6824bfc19557fc
//IV = 6f58a93fe1d207fae4ed2f6d
//PT = cc38bccd6bc536ad919b1395f5d63801f99f8068d65ca5ac63872daf16b93901
//AAD = 021fafd238463973ffe80256e5b1c6b1
//CT = dfce4e9cd291103d7fe4e63351d9e79d3dfd391e3267104658212da96521b7db
//Tag = 542465ef599316f73a7a560509a2d9f2
//
// keylen = 128, ivlen = 96, ptlen = 256, aadlen = 128, taglen = 128, count = 0
TEST(Main,GCMVS1ViaFixed32BTextSizeWITHWORKSPACE)
{
    // Inputs to encryption.
    static const uint8_t input[32] = { 0xcc, 0x38, 0xbc, 0xcd, 0x6b, 0xc5, 0x36, 0xad, 0x91, 0x9b, 0x13, 0x95, 0xf5, 0xd6, 0x38, 0x01, 0xf9, 0x9f, 0x80, 0x68, 0xd6, 0x5c, 0xa5, 0xac, 0x63, 0x87, 0x2d, 0xaf, 0x16, 0xb9, 0x39, 0x01 };
    static const uint8_t key[AES_KEY_SIZE/8] = { 0x29, 0x8e, 0xfa, 0x1c, 0xcf, 0x29, 0xcf, 0x62, 0xae, 0x68, 0x24, 0xbf, 0xc1, 0x95, 0x57, 0xfc };
    static const uint8_t nonce[GCM_NONCE_LENGTH] = { 0x6f, 0x58, 0xa9, 0x3f, 0xe1, 0xd2, 0x07, 0xfa, 0xe4, 0xed, 0x2f, 0x6d };
    static const uint8_t aad[16] = { 0x02, 0x1f, 0xaf, 0xd2, 0x38, 0x46, 0x39, 0x73, 0xff, 0xe8, 0x02, 0x56, 0xe5, 0xb1, 0xc6, 0xb1 };
    // Space for outputs from encryption.
    uint8_t tag[GCM_TAG_LENGTH]; // Space for tag.
    uint8_t cipherText[OTV0P2BASE::fnmax(32, (int)sizeof(input))]; // Space for encrypted text.
    // Create a workspace big enough for any operation.
    constexpr size_t workspaceRequired = OTAESGCM::OTAES128GCMGenericWithWorkspace<>::workspaceRequired;
    uint8_t workspace[workspaceRequired];
    memset(workspace, 0, sizeof(workspace));
    // Do encryption via simplified interface.
    ASSERT_TRUE(OTAESGCM::fixed32BTextSize12BNonce16BTagSimpleEnc_DEFAULT_WITH_LWORKSPACE(
            workspace, sizeof(workspace),
            key, nonce,
            aad, sizeof(aad),
            input,
            cipherText, tag));
    // Security: ensure that no part of the workspace has been left unzeroed.
    for(int i = workspaceRequired; --i >= 0; ) { ASSERT_EQ(0, workspace[i]); }
    // Check some of the cipher text and tag.
    //            "0388DACE60B6A392F328C2B971B2FE78F795AAAB494B5923F7FD89FF948B  61 47 72 C7 92 9C D0 DD 68 1B D8 A3 7A 65 6F 33" :
    EXPECT_EQ(0xdf, cipherText[0]);
    EXPECT_EQ(0x91, cipherText[5]);
    EXPECT_EQ(0xdb, cipherText[sizeof(cipherText)-1]);
    EXPECT_EQ(0x24, tag[1]);
    EXPECT_EQ(0xd9, tag[14]);
    // Decrypt via simplified interface...
    uint8_t inputDecoded[32];
    EXPECT_TRUE(OTAESGCM::fixed32BTextSize12BNonce16BTagSimpleDec_DEFAULT_WITH_LWORKSPACE(
            workspace, workspaceRequired,
            key, nonce,
            aad, sizeof(aad),
            cipherText, tag,
            inputDecoded));
    // Security: ensure that no part of the workspace has been left unzeroed.
    for(int i = workspaceRequired; --i >= 0; ) { ASSERT_EQ(0, workspace[i]); }
    EXPECT_EQ(0, memcmp(input, inputDecoded, 32));
    // Try enc/auth with no (ie zero-length) plaintext.
    EXPECT_TRUE(OTAESGCM::fixed32BTextSize12BNonce16BTagSimpleEnc_DEFAULT_WITH_LWORKSPACE(
            workspace, workspaceRequired,
            key, nonce,
            aad, sizeof(aad),
            NULL,
            cipherText, tag));
    // Security: ensure that no part of the workspace has been left unzeroed.
    for(int i = workspaceRequired; --i >= 0; ) { ASSERT_EQ(0, workspace[i]); }
    // Check some of the tag.
    EXPECT_EQ(0x57, tag[1]);
    EXPECT_EQ(0x25, tag[14]);
    // Auth/decrypt (auth should still succeed).
    EXPECT_TRUE(OTAESGCM::fixed32BTextSize12BNonce16BTagSimpleDec_DEFAULT_WITH_LWORKSPACE(
            workspace, workspaceRequired,
            key, nonce,
            aad, sizeof(aad),
            NULL, tag,
            inputDecoded));
    // Security: ensure that no part of the workspace has been left unzeroed.
    for(int i = workspaceRequired; --i >= 0; ) { ASSERT_EQ(0, workspace[i]); }
    // Check that too-small or NULL workspaces are rejected,
    // and that oversize ones are accepted.
    // Encrypt...
    EXPECT_FALSE(OTAESGCM::fixed32BTextSize12BNonce16BTagSimpleEnc_DEFAULT_WITH_LWORKSPACE(
            NULL, workspaceRequired,
            key, nonce,
            aad, sizeof(aad),
            input,
            cipherText, tag)) << "Workspace NULL but nominally correct size, should fail";
    EXPECT_FALSE(OTAESGCM::fixed32BTextSize12BNonce16BTagSimpleEnc_DEFAULT_WITH_LWORKSPACE(
            workspace, OTAESGCM::OTAES128GCMGenericWithWorkspace<>::workspaceRequiredEnc-1,
            key, nonce,
            aad, sizeof(aad),
            input,
            cipherText, tag)) << "Workspace one byte too small should fail: " << (workspaceRequired-1);
    EXPECT_FALSE(OTAESGCM::fixed32BTextSize12BNonce16BTagSimpleEnc_DEFAULT_WITH_LWORKSPACE(
            workspace, 0,
            key, nonce,
            aad, sizeof(aad),
            input,
            cipherText, tag)) << "Workspace zero length should fail: " << (workspaceRequired-1);;
    EXPECT_TRUE(OTAESGCM::fixed32BTextSize12BNonce16BTagSimpleEnc_DEFAULT_WITH_LWORKSPACE(
            workspace, OTAESGCM::OTAES128GCMGenericWithWorkspace<>::workspaceRequiredEnc+1,
            key, nonce,
            aad, sizeof(aad),
            input,
            cipherText, tag));
    // Decrypt..
    EXPECT_FALSE(OTAESGCM::fixed32BTextSize12BNonce16BTagSimpleDec_DEFAULT_WITH_LWORKSPACE(
            NULL, workspaceRequired,
            key, nonce,
            aad, sizeof(aad),
            cipherText, tag,
            inputDecoded));
    EXPECT_FALSE(OTAESGCM::fixed32BTextSize12BNonce16BTagSimpleDec_DEFAULT_WITH_LWORKSPACE(
            workspace, workspaceRequired-1,
            key, nonce,
            aad, sizeof(aad),
            cipherText, tag,
            inputDecoded)) << (workspaceRequired-1);
    EXPECT_FALSE(OTAESGCM::fixed32BTextSize12BNonce16BTagSimpleDec_DEFAULT_WITH_LWORKSPACE(
            workspace, 0,
            key, nonce,
            aad, sizeof(aad),
            cipherText, tag,
            inputDecoded));
    EXPECT_TRUE(OTAESGCM::fixed32BTextSize12BNonce16BTagSimpleDec_DEFAULT_WITH_LWORKSPACE(
            workspace, workspaceRequired+1,
            key, nonce,
            aad, sizeof(aad),
            cipherText, tag,
            inputDecoded));
}


// Test encoding/encryption then decoding/decryption of entire secure frame.
//
// DHD20161107: imported from test_SECFRAME.ino testSecureSmallFrameEncoding().
TEST(OTAESGCMSecureFrame, SecureSmallFrameEncoding)
{
    // workspaces
    constexpr size_t encWorkspaceSize = OTRadioLink::SimpleSecureFrame32or0BodyTXBase::encodeRaw_total_scratch_usage_OTAESGCM_2p0;
    EXPECT_EQ(272U, encWorkspaceSize);
    uint8_t encWorkspace[encWorkspaceSize];
    OTV0P2BASE::ScratchSpaceL sWEnc(encWorkspace, sizeof(encWorkspace));
    constexpr size_t decWorkspaceSize =
            OTRadioLink::SimpleSecureFrame32or0BodyRXBase::decodeRaw_total_scratch_usage_OTAESGCM_3p0
            + OTAESGCM::OTAES128GCMGenericWithWorkspace<>::workspaceRequiredDec;
    EXPECT_EQ(320U, decWorkspaceSize);
    uint8_t decWorkspace[decWorkspaceSize];
    OTV0P2BASE::ScratchSpaceL sWDec(decWorkspace, sizeof(decWorkspace));


    uint8_t _buf[OTRadioLink::SecurableFrameHeader::maxSmallFrameSize];
    OTRadioLink::OTBuf_t buf(_buf, sizeof(_buf));
    //Example 3: secure, no valve, representative minimum stats {"b":1}).
    //Note that the sequence number must match the 4 lsbs of the message count, ie from iv[11].
    //and the ID is 0xaa 0xaa 0xaa 0xaa (transmitted) with the next ID bytes 0x55 0x55.
    //ResetCounter = 42
    //TxMsgCounter = 793
    //(Thus nonce/IV: aa aa aa aa 55 55 00 00 2a 00 03 19)
    //
    //3e cf 94 aa aa aa aa 20 | b3 45 f9 29 69 57 0c b8 28 66 14 b4 f0 69 b0 08 71 da d8 fe 47 c1 c3 53 83 48 88 03 7d 58 75 75 | 00 00 2a 00 03 19 29 3b 31 52 c3 26 d2 6d d0 8d 70 1e 4b 68 0d cb 80
    //
    //3e  length of header (62) after length byte 5 + (encrypted) body 32 + trailer 32
    //cf  'O' secure OpenTRV basic frame
    //04  0 sequence number, ID length 4
    //aa  ID byte 1
    //aa  ID byte 2
    //aa  ID byte 3
    //aa  ID byte 4
    //20  body length 32 (after padding and encryption)
    //    Plaintext body (length 8): 0x7f 0x11 { " b " : 1
    //    Padded: 7f 11 7b 22 62 22 3a 31 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 17
    //b3 45 f9 ... 58 75 75  32 bytes of encrypted body
    //00 00 2a  reset counter
    //00 03 19  message counter
    //29 3b 31 ... 68 0d cb  16 bytes of authentication tag
    //80  enc/auth type/format indicator.
    // Preshared ID prefix; only an initial part/prefix of this goes on the wire in the header.
    uint8_t id[] = { 0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55 };
    OTRadioLink::OTBuf_t id4bytes(id, 4);
    // IV/nonce starting with first 6 bytes of preshared ID, then 6 bytes of counter.
    const uint8_t iv[] = { 0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x00, 0x00, 0x2a, 0x00, 0x03, 0x19 };
    // 'O' frame body with some JSON stats.
    const uint8_t body[] = { 0x7f, 0x11, 0x7b, 0x22, 0x62, 0x22, 0x3a, 0x31 };
    uint8_t _bodyBuf[32] = {};
    OTRadioLink::OTBuf_t bodyBuf(_bodyBuf, sizeof(_bodyBuf));
    memcpy(bodyBuf.buf, body, sizeof(body));

    OTRadioLink::OTEncodeData_T fdTX(_bodyBuf, sizeof(_bodyBuf), _buf, sizeof(_buf));
    fdTX.ptextLen = sizeof(body);
    fdTX.fType = OTRadioLink::FTS_BasicSensorOrValve;
    const uint8_t encodedLength = OTRadioLink::SimpleSecureFrame32or0BodyTXBase::encodeRaw(
                                        fdTX,
                                        id4bytes.buf,
                                        id4bytes.bufsize,
                                        iv,
                                        OTAESGCM::fixed32BTextSize12BNonce16BTagSimpleEnc_DEFAULT_WITH_LWORKSPACE,
                                        sWEnc, zeroBlock);
    EXPECT_EQ(63, encodedLength);
    EXPECT_TRUE(encodedLength <= buf.bufsize);
    //3e cf 04 aa aa aa aa 20 | ...
    EXPECT_EQ(0x3e, buf.buf[0]);
    EXPECT_EQ(0xcf, buf.buf[1]);
    EXPECT_EQ(0x94, buf.buf[2]); // Seq num is iv[11] & 0xf, ie 4 lsbs of message counter (and IV).
    EXPECT_EQ(0xaa, buf.buf[3]);
    EXPECT_EQ(0xaa, buf.buf[4]);
    EXPECT_EQ(0xaa, buf.buf[5]);
    EXPECT_EQ(0xaa, buf.buf[6]);
    EXPECT_EQ(0x20, buf.buf[7]);
    //... b3 45 f9 29 69 57 0c b8 28 66 14 b4 f0 69 b0 08 71 da d8 fe 47 c1 c3 53 83 48 88 03 7d 58 75 75 | ...
    EXPECT_EQ(0xb3, buf.buf[8]); // 1st byte of encrypted body.
    EXPECT_EQ(0x75, buf.buf[39]); // 32nd/last byte of encrypted body.
    //... 00 00 2a 00 03 19 29 3b 31 52 c3 26 d2 6d d0 8d 70 1e 4b 68 0d cb 80
    EXPECT_EQ(0x00, buf.buf[40]); // 1st byte of counters.
    EXPECT_EQ(0x00, buf.buf[41]);
    EXPECT_EQ(0x2a, buf.buf[42]);
    EXPECT_EQ(0x00, buf.buf[43]);
    EXPECT_EQ(0x03, buf.buf[44]);
    EXPECT_EQ(0x19, buf.buf[45]); // Last byte of counters.
    EXPECT_EQ(0x29, buf.buf[46]); // 1st byte of tag.
    EXPECT_EQ(0xcb, buf.buf[61]); // 16th/last byte of tag.
    EXPECT_EQ(0x80, buf.buf[62]); // enc format.

    // (Nominally a longer ID and key is looked up with the ID in the header, and an iv built.)
    uint8_t decryptedBodyOut[OTRadioLink::ENC_BODY_SMALL_FIXED_PTEXT_MAX_SIZE];
    // To decode, emulating RX, structurally validate unpack the header and extract the ID.
    OTRadioLink::OTDecodeData_T fdRX(buf.buf, decryptedBodyOut);
    EXPECT_TRUE(0 != fdRX.sfh.decodeHeader(buf.buf, encodedLength));
    // Should decode and authenticate correctly.
    EXPECT_TRUE(0 != OTRadioLink::SimpleSecureFrame32or0BodyRXBase::decodeRaw(
                        fdRX,
                        OTAESGCM::fixed32BTextSize12BNonce16BTagSimpleDec_DEFAULT_WITH_LWORKSPACE,
                        sWDec, zeroBlock, iv));
    // Body content should be correctly decrypted and extracted.
    EXPECT_EQ(sizeof(body), fdRX.ptextLen);
    EXPECT_EQ(0, memcmp(body, decryptedBodyOut, sizeof(body)));

    // Using ASSERT to avoid cryptic crash message (Floating point exception (core dumped)) when encodedLength is 0.
    ASSERT_NE(0, encodedLength);
    // Check that flipping any single bit should make the decode fail
    // unless it leaves all info (seqNum, id, body) untouched.
    const uint8_t loc = OTV0P2BASE::randRNG8() % encodedLength;
    const uint8_t mask = (uint8_t) (0x100U >> (1+(OTV0P2BASE::randRNG8()&7)));
    buf.buf[loc] ^= mask;
    //  Serial.println(loc);
    //  Serial.println(mask);
    EXPECT_TRUE(
            (0 == fdRX.sfh.decodeHeader(buf.buf, encodedLength)) ||
            (0 == OTRadioLink::SimpleSecureFrame32or0BodyRXBase::decodeRaw(fdRX,
                                        OTAESGCM::fixed32BTextSize12BNonce16BTagSimpleDec_DEFAULT_WITH_LWORKSPACE,
                                        sWDec, zeroBlock, iv)) ||
            ((sizeof(body) == fdRX.ptextLen) && (0 == memcmp(body, decryptedBodyOut, sizeof(body))) && (0 == memcmp(id, fdRX.sfh.id, 4)))
    );
}

// Test encoding of beacon frames.
// FIXME No direct secure beacon function yet.
// DHD20161107: imported from test_SECFRAME.ino testBeaconEncoding().
TEST(OTAESGCMSecureFrame, BeaconEncodingWithWorkspace)
{
    // workspaces
    constexpr size_t encWorkspaceSize = OTRadioLink::SimpleSecureFrame32or0BodyTXBase::encodeRaw_total_scratch_usage_OTAESGCM_2p0;
    uint8_t encWorkspace[encWorkspaceSize];
    OTV0P2BASE::ScratchSpaceL sWEnc(encWorkspace, sizeof(encWorkspace));
    constexpr size_t decWorkspaceSize =
            OTRadioLink::SimpleSecureFrame32or0BodyRXBase::decodeRaw_total_scratch_usage_OTAESGCM_3p0
            + + OTAESGCM::OTAES128GCMGenericWithWorkspace<>::workspaceRequiredDec;;
    uint8_t decWorkspace[decWorkspaceSize];
    OTV0P2BASE::ScratchSpaceL sWDec(decWorkspace, sizeof(decWorkspace));

    // Non-secure beacon.
    uint8_t buf[OTV0P2BASE::fnmax(OTRadioLink::generateNonsecureBeaconMaxBufSize, OTRadioLink::SimpleSecureFrame32or0BodyTXBase::generateSecureBeaconMaxBufSize)];
    OTRadioLink::OTBuf_t otbuf(buf, sizeof(buf));
    OTRadioLink::OTBuf_t nullbuf(nullptr, 0);
    uint8_t zeroBufBlock[OTRadioLink::SecurableFrameHeader::maxIDLength] = {};
    OTRadioLink::OTBuf_t zeroBuf(zeroBufBlock, sizeof(zeroBufBlock));
    // Generate zero-length-ID beacon.
    const uint8_t b0 = OTRadioLink::generateNonsecureBeacon(otbuf, 0, nullbuf.buf, nullbuf.bufsize);
    EXPECT_EQ(5, b0);
    EXPECT_EQ(0x04, buf[0]);
    EXPECT_EQ(0x21, buf[1]);
    EXPECT_EQ(0x00, buf[2]);
    EXPECT_EQ(0x00, buf[3]); // Body length 0.
    EXPECT_EQ(0x65, buf[4]);
    // Generate maximum-length-zero-ID beacon automatically at non-zero seq.
    const uint8_t b1 = OTRadioLink::generateNonsecureBeacon(otbuf, 4, zeroBuf.buf, zeroBuf.bufsize);
    EXPECT_EQ(13, b1);
    EXPECT_EQ(0x0c, buf[0]);
    EXPECT_EQ(0x21, buf[1]);
    EXPECT_EQ(0x48, buf[2]);
    EXPECT_EQ(0x00, buf[3]);
    EXPECT_EQ(0x00, buf[4]);
    EXPECT_EQ(0x00, buf[5]);
    EXPECT_EQ(0x00, buf[6]);
    EXPECT_EQ(0x00, buf[7]);
    EXPECT_EQ(0x00, buf[8]);
    EXPECT_EQ(0x00, buf[9]);
    EXPECT_EQ(0x00, buf[10]);
    EXPECT_EQ(0x00, buf[11]); // Body length 0.
    EXPECT_EQ(0x29, buf[12]);
    #if 0 // DISABLED AS DEPENDS ON AVR ARCH
    // Generate maximum-length-from-EEPROM-ID beacon automatically at non-zero seq.
    const uint8_t b2 = OTRadioLink::generateNonsecureBeacon(buf, sizeof(buf), 5, NULL, OTRadioLink::SecurableFrameHeader::maxIDLength);
    EXPECT_EQ(13, b2);
    EXPECT_EQ(0x0c, buf[0]);
    EXPECT_EQ(0x21, buf[1]);
    EXPECT_EQ(0x58, buf[2]);
    for(uint8_t i = 0; i < OTRadioLink::SecurableFrameHeader::maxIDLength; ++i)
    { EXPECT_EQ(eeprom_read_byte((uint8_t *)(V0P2BASE_EE_START_ID + i)), buf[3 + i]); }
    EXPECT_EQ(0x00, buf[11]); // Body length 0.
    #endif

    for(uint8_t idLen = 0; idLen <= 8; ++idLen)
    {
        // Secure beacon...  All zeros key; ID and IV as from spec Example 3 at 20160207.
        const uint8_t *const key = zeroBlock;
        // Preshared ID prefix; only an initial part/prefix of this goes on the wire in the header.
        uint8_t _id[] = { 0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55 };
        OTRadioLink::OTBuf_t id(_id, idLen);
        OTRadioLink::OTBuf_t body(nullptr, 0);
        // IV/nonce starting with first 6 bytes of preshared ID, then 6 bytes of counter.
        const uint8_t iv[] = { 0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x00, 0x00, 0x2a, 0x00, 0x03, 0x19 };

        OTRadioLink::OTEncodeData_T fdEnc(body.buf, body.bufsize, otbuf.buf, otbuf.bufsize);
        fdEnc.fType = OTRadioLink::FTS_ALIVE;
        //    const uint8_t sb1 = OTRadioLink::SimpleSecureFrame32or0BodyTXBase::generateSecureBeaconRaw(buf, sizeof(buf), id, idLen, iv, OTAESGCM::fixed32BTextSize12BNonce16BTagSimpleEnc_DEFAULT_STATELESS, NULL, key);
        const uint8_t sb1 = OTRadioLink::SimpleSecureFrame32or0BodyTXBase::encodeRaw(
                                        fdEnc,
                                        id.buf,
                                        id.bufsize,
                                        iv,
                                        OTAESGCM::fixed32BTextSize12BNonce16BTagSimpleEnc_DEFAULT_WITH_LWORKSPACE,
                                        sWEnc, key);

        EXPECT_EQ(27 + idLen, sb1);
        //
        // Check decoding (auth/decrypt) of beacon at various levels.
        // Validate structure of frame first.
        // This is quick and checks for insane/dangerous values throughout.
        OTRadioLink::OTDecodeData_T fd(buf, body.buf);
        const uint8_t l = fd.sfh.decodeHeader(buf, sb1 + 1);
        EXPECT_EQ(4 + idLen, l);
        const uint8_t dlr = OTRadioLink::SimpleSecureFrame32or0BodyRXBase::decodeRaw(fd,
                                        OTAESGCM::fixed32BTextSize12BNonce16BTagSimpleDec_DEFAULT_WITH_LWORKSPACE,
                                        sWDec, key, iv);
        // Should be able to decode, ie pass authentication.
        EXPECT_EQ(27 + idLen, dlr);
        //    // Construct IV from ID and trailer.
        //    const uint8_t dlfi = OTRadioLink::SimpleSecureFrame32or0BodyRXV0p2::getInstance()._decodeSecureSmallFrameFromID(&sfh,
        //                                    buf, sizeof(buf),
        //                                    OTAESGCM::fixed32BTextSize12BNonce16BTagSimpleDec_DEFAULT_STATELESS,
        //                                    id, sizeof(id),
        //                                    NULL, key,
        //                                    NULL, 0, decryptedBodyOutSize);
        //    // Should be able to decode, ie pass authentication.
        //    EXPECT_EQ(27 + idLen, dlfi);
    }
    //  const unsigned long after = millis();
    //  Serial.println(after - before); // DHD20160207: 1442 for 8 rounds, or ~180ms per encryption.
}

#if 0 // DISABLED AS DEPENDS ON AVR ARCH
// Test some basic parameters of node associations.
// Does not wear non-volatile memory (eg EEPROM).
//
// DHD20161107: imported from test_SECFRAME.ino testNodeAssoc().
TEST(OTAESGCMSecureFrame, NodeAssoc)
  {
  const uint8_t nas = OTV0P2BASE::countNodeAssociations();
  EXPECT_TRUE(nas <= OTV0P2BASE::MAX_NODE_ASSOCIATIONS);
  const int8_t i = OTV0P2BASE::getNextMatchingNodeID(0, NULL, 0, NULL);
  // Zero-length prefix look-up should succeed IFF there is at least one entry.
  EXPECT_TRUE((0 == nas) == (-1 == i));
  }
#endif

// Test some message counter routines.
// Does not wear non-volatile memory (eg EEPROM).
//
// DHD20161107: imported from test_SECFRAME.ino testMsgCount().
TEST(OTAESGCMSecureFrame, MsgCount)
{
    // Two counter values to compare that should help spot overflow or wrong byte order operations.
    const uint8_t count1[] = { 0, 0, 0x83, 0, 0, 0 };
    const uint8_t count1plus1[] = { 0, 0, 0x83, 0, 0, 1 };
    const uint8_t count1plus256[] = { 0, 0, 0x83, 0, 1, 0 };
    const uint8_t count2[] = { 0, 0, 0x82, 0x88, 1, 1 };
    const uint8_t countmax[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
    // Check that identical values compare as identical.
    EXPECT_EQ(0, OTRadioLink::SimpleSecureFrame32or0BodyRXBase::msgcountercmp(zeroBlock, zeroBlock));
    EXPECT_EQ(0, OTRadioLink::SimpleSecureFrame32or0BodyRXBase::msgcountercmp(count1, count1));
    EXPECT_EQ(0, OTRadioLink::SimpleSecureFrame32or0BodyRXBase::msgcountercmp(count2, count2));
    EXPECT_TRUE(OTRadioLink::SimpleSecureFrame32or0BodyRXBase::msgcountercmp(count1, count2) > 0);
    EXPECT_TRUE(OTRadioLink::SimpleSecureFrame32or0BodyRXBase::msgcountercmp(count2, count1) < 0);
    // Test simple addition to counts.
    uint8_t count1copy[sizeof(count1)];
    memcpy(count1copy, count1, sizeof(count1copy));
    EXPECT_TRUE(OTRadioLink::SimpleSecureFrame32or0BodyRXBase::msgcounteradd(count1copy, 0));
    EXPECT_EQ(0, OTRadioLink::SimpleSecureFrame32or0BodyRXBase::msgcountercmp(count1copy, count1));
    EXPECT_TRUE(OTRadioLink::SimpleSecureFrame32or0BodyRXBase::msgcounteradd(count1copy, 1));
    EXPECT_EQ(0, OTRadioLink::SimpleSecureFrame32or0BodyRXBase::msgcountercmp(count1copy, count1plus1));
    EXPECT_TRUE(OTRadioLink::SimpleSecureFrame32or0BodyRXBase::msgcounteradd(count1copy, 255));
    EXPECT_EQ(0, OTRadioLink::SimpleSecureFrame32or0BodyRXBase::msgcountercmp(count1copy, count1plus256));
    // Test simple addition to counts.
    uint8_t countmaxcopy[sizeof(countmax)];
    memcpy(countmaxcopy, countmax, sizeof(countmaxcopy));
    EXPECT_TRUE(OTRadioLink::SimpleSecureFrame32or0BodyRXBase::msgcounteradd(countmaxcopy, 0));
    EXPECT_EQ(0, OTRadioLink::SimpleSecureFrame32or0BodyRXBase::msgcountercmp(countmaxcopy, countmax));
    EXPECT_TRUE(!OTRadioLink::SimpleSecureFrame32or0BodyRXBase::msgcounteradd(countmaxcopy, 1));
    EXPECT_EQ(0, OTRadioLink::SimpleSecureFrame32or0BodyRXBase::msgcountercmp(countmaxcopy, countmax));
    EXPECT_TRUE(!OTRadioLink::SimpleSecureFrame32or0BodyRXBase::msgcounteradd(countmaxcopy, 42));
    EXPECT_EQ(0, OTRadioLink::SimpleSecureFrame32or0BodyRXBase::msgcountercmp(countmaxcopy, countmax));
}

#if 0 // DISABLED AS DEPENDS ON AVR ARCH
// Test handling of persistent/reboot/restart part of primary message counter.
// Does not wear non-volatile memory (eg EEPROM).
//
// DHD20161107: imported from test_SECFRAME.ino testPermMsgCount().
TEST(OTAESGCMSecureFrame, PermMsgCount)
  {
  uint8_t loadBuf[OTV0P2BASE::VOP2BASE_EE_LEN_PERSISTENT_MSG_RESTART_CTR];
  uint8_t buf[OTRadioLink::SimpleSecureFrame32or0BodyTXBase::primaryPeristentTXMessageRestartCounterBytes];
  // Initialise to state of empty EEPROM; result should be a valid all-zeros restart count.
  memset(loadBuf, 0, sizeof(loadBuf));
  EXPECT_TRUE(OTRadioLink::SimpleSecureFrame32or0BodyTXV0p2::read3BytePersistentTXRestartCounter(loadBuf, buf));
  EXPECT_EQ(0, memcmp(buf, zeroBlock, OTRadioLink::SimpleSecureFrame32or0BodyTXBase::primaryPeristentTXMessageRestartCounterBytes));
  // Ensure that it can be incremented and gives the correct next (0x000001) value.
  EXPECT_TRUE(OTRadioLink::SimpleSecureFrame32or0BodyTXV0p2::incrementTXNVCtrPrefix(loadBuf));
  EXPECT_TRUE(OTRadioLink::SimpleSecureFrame32or0BodyTXV0p2::read3BytePersistentTXRestartCounter(loadBuf, buf));
  EXPECT_EQ(0, memcmp(buf, zeroBlock, OTRadioLink::SimpleSecureFrame32or0BodyTXBase::primaryPeristentTXMessageRestartCounterBytes - 1));
  EXPECT_EQ(1, buf[2]);
  // Initialise to all-0xff state (with correct CRC), which should cause failure.
  memset(loadBuf, 0xff, sizeof(loadBuf)); loadBuf[3] = 0xf; loadBuf[7] = 0xf;
  EXPECT_TRUE(!OTRadioLink::SimpleSecureFrame32or0BodyTXV0p2::read3BytePersistentTXRestartCounter(loadBuf, buf));
  // Ensure that it CANNOT be incremented.
  EXPECT_TRUE(!OTRadioLink::SimpleSecureFrame32or0BodyTXV0p2::incrementTXNVCtrPrefix(loadBuf));
  // Test recovery from broken primary counter a few times.
  memset(loadBuf, 0, sizeof(loadBuf));
  for(uint8_t i = 0; i < 3; ++i)
    {
    // Damage one of the primary or secondary counter bytes (or CRCs).
    loadBuf[0x7 & OTV0P2BASE::randRNG8()] = OTV0P2BASE::randRNG8();
    EXPECT_TRUE(OTRadioLink::SimpleSecureFrame32or0BodyTXV0p2::getInstance().read3BytePersistentTXRestartCounter(loadBuf, buf));
    EXPECT_EQ(0, buf[1]);
    EXPECT_EQ(0, buf[1]);
    EXPECT_EQ(i, buf[2]);
    EXPECT_TRUE(OTRadioLink::SimpleSecureFrame32or0BodyTXV0p2::getInstance().incrementTXNVCtrPrefix(loadBuf));
    }
  // Also verify in passing that all zero message counter will never be acceptable for an RX message,
  // regardless of the node ID, since the new count as to be higher than any previous for the ID.
  EXPECT_TRUE(!OTRadioLink::SimpleSecureFrame32or0BodyRXV0p2::getInstance().validateRXMessageCount(zeroBlock, zeroBlock));
  }
#endif

#if 0 // DISABLED AS DEPENDS ON AVR ARCH
// Test handling of persistent/reboot/restart part of primary TX message counter.
// Tests to only be run once per restart because they cause device wear (EEPROM).
// NOTE: best not to run on a live device as this will mess with its associations, etc.
// This clears the key so as to make it clear that this device is not to be regarded as secure.
//
// DHD20161107: imported from test_SECFRAME.ino testPermMsgCountRunOnce().
TEST(OTAESGCMSecureFrame, PermMsgCountRunOnce)
  {
  // We're compromising system security and keys here, so clear any secret keys set, first.
  EXPECT_TRUE(OTV0P2BASE::setPrimaryBuilding16ByteSecretKey(NULL)); // Fail if we can't ensure that the key is cleared.
  // Check that key is correctly erased.
  uint8_t tmpKeyBuf[16];
  EXPECT_TRUE(!OTV0P2BASE::getPrimaryBuilding16ByteSecretKey(tmpKeyBuf));
  memset(tmpKeyBuf, 0, sizeof(tmpKeyBuf));
  // Check that we don't get a spurious key match.
  EXPECT_TRUE(!OTV0P2BASE::checkPrimaryBuilding16ByteSecretKey(tmpKeyBuf));
  OTRadioLink::SimpleSecureFrame32or0BodyTXV0p2 instance = OTRadioLink::SimpleSecureFrame32or0BodyTXV0p2::getInstance();
  // Working buffer space...
  uint8_t loadBuf[OTV0P2BASE::VOP2BASE_EE_LEN_PERSISTENT_MSG_RESTART_CTR];
  uint8_t buf[OTRadioLink::SimpleSecureFrame32or0BodyTXBase::primaryPeristentTXMessageRestartCounterBytes];
  // Initial test that blank EEPROM (or reset to all zeros) after processing yields all zeros.
  EXPECT_TRUE(instance.resetRaw3BytePersistentTXRestartCounterInEEPROM(true));
  instance.loadRaw3BytePersistentTXRestartCounterFromEEPROM(loadBuf);
  EXPECT_TRUE(0 == memcmp(loadBuf, zeroBlock, sizeof(loadBuf)));
  EXPECT_TRUE(instance.read3BytePersistentTXRestartCounter(loadBuf, buf));
  EXPECT_EQ(0, memcmp(buf, zeroBlock, OTRadioLink::SimpleSecureFrame32or0BodyTXBase::primaryPeristentTXMessageRestartCounterBytes));
  EXPECT_TRUE(instance.getTXNVCtrPrefix(buf));
  EXPECT_TRUE(0 == memcmp(buf, zeroBlock, OTRadioLink::SimpleSecureFrame32or0BodyTXBase::primaryPeristentTXMessageRestartCounterBytes));
  // Increment the persistent TX counter and ensure that we see it as non-zero.
  EXPECT_TRUE(instance.incrementTXNVCtrPrefix());
  EXPECT_TRUE(instance.getTXNVCtrPrefix(buf));
  EXPECT_TRUE(0 != memcmp(buf, zeroBlock, OTRadioLink::SimpleSecureFrame32or0BodyTXBase::primaryPeristentTXMessageRestartCounterBytes));
  // So reset to non-all-zeros (should be default) and make sure that we see it as not-all-zeros.
  EXPECT_TRUE(instance.resetRaw3BytePersistentTXRestartCounterInEEPROM());
  EXPECT_TRUE(instance.getTXNVCtrPrefix(buf));
  EXPECT_TRUE(0 != memcmp(buf, zeroBlock, OTRadioLink::SimpleSecureFrame32or0BodyTXBase::primaryPeristentTXMessageRestartCounterBytes));
  // Initial test that from blank EEPROM (or reset to all zeros)
  // that getting the message counter gives non-zero reboot and ephemeral parts.
  EXPECT_TRUE(instance.resetRaw3BytePersistentTXRestartCounterInEEPROM(true));
  uint8_t mcbuf[OTRadioLink::SimpleSecureFrame32or0BodyBase::fullMessageCounterBytes];
  EXPECT_TRUE(instance.getNextTXMsgCtr(mcbuf));
#if 0
  for(int i = 0; i < sizeof(mcbuf); ++i) { Serial.print(' '); Serial.print(mcbuf[i], HEX); }
  Serial.println();
#endif
  // Assert that each half of the message counter is non-zero.
  EXPECT_TRUE((0 != mcbuf[0]) || (0 != mcbuf[1]) || (0 != mcbuf[2]));
  EXPECT_TRUE((0 != mcbuf[3]) || (0 != mcbuf[4]) || (0 != mcbuf[5]));
  }
#endif

#if 0 // DISABLED AS DEPENDS ON AVR ARCH
// Test some basic parameters of node associations.
// Also tests intimate interaction with management of RX message counters.
// Tests to only be run once per restart because they cause device wear (EEPROM).
// NOTE: best not to run on a live device as this will mess with its associations, etc.
//
// DHD20161107: imported from test_SECFRAME.ino testNodeAssocRunOnce().
TEST(OTAESGCMSecureFrame, NodeAssocRunOnce)
  {
  OTV0P2BASE::clearAllNodeAssociations();
  EXPECT_EQ(0, OTV0P2BASE::countNodeAssociations());
  EXPECT_EQ(-1, OTV0P2BASE::getNextMatchingNodeID(0, NULL, 0, NULL));
  // Check that attempting to get aux data for a non-existent node/association fails.
  uint8_t mcbuf[OTRadioLink::SimpleSecureFrame32or0BodyBase::fullMessageCounterBytes];
  EXPECT_TRUE(!OTRadioLink::SimpleSecureFrame32or0BodyRXV0p2::getInstance().getLastRXMessageCounter(zeroBlock, mcbuf));
  // Test adding associations and looking them up.
  const uint8_t ID0[] = { 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7 };
  EXPECT_EQ(0, OTV0P2BASE::addNodeAssociation(ID0));
  EXPECT_EQ(1, OTV0P2BASE::countNodeAssociations());
  EXPECT_EQ(0, OTV0P2BASE::getNextMatchingNodeID(0, NULL, 0, NULL));
  for(uint8_t i = 0; i <= sizeof(ID0); ++i)
    { EXPECT_EQ(0, OTV0P2BASE::getNextMatchingNodeID(0, ID0, i, NULL)); }
  // Check that RX msg count for new association is OK, and all zeros.
  EXPECT_TRUE(OTRadioLink::SimpleSecureFrame32or0BodyRXV0p2::getInstance().getLastRXMessageCounter(ID0, mcbuf));
  EXPECT_EQ(0, memcmp(mcbuf, zeroBlock, sizeof(mcbuf)));
  // Now deliberately damage the primary count and check that the counter value can still be retrieved.
  // Ensure damanged byte has at least one 1 and one 0 and is at a random position towards the end.
  OTV0P2BASE::eeprom_smart_update_byte((uint8_t *)(OTV0P2BASE::V0P2BASE_EE_START_NODE_ASSOCIATIONS + OTV0P2BASE::V0P2BASE_EE_NODE_ASSOCIATIONS_MSG_CNT_0_OFFSET + 2 + (3 & OTV0P2BASE::randRNG8())), 0x40 | (0x3f & OTV0P2BASE::randRNG8()));
  EXPECT_TRUE(OTRadioLink::SimpleSecureFrame32or0BodyRXV0p2::getInstance().getLastRXMessageCounter(ID0, mcbuf));
  EXPECT_EQ(0, memcmp(mcbuf, zeroBlock, sizeof(mcbuf)));
  // Attempting to update to the same (0) value should fail.
  EXPECT_TRUE(!OTRadioLink::SimpleSecureFrame32or0BodyRXV0p2::getInstance().updateRXMessageCountAfterAuthentication(ID0, zeroBlock));
  // Updating to a new count and reading back should work.
  const uint8_t newCount[] = { 0, 1, 2, 3, 4, 5 };
  EXPECT_TRUE(OTRadioLink::SimpleSecureFrame32or0BodyRXV0p2::getInstance().updateRXMessageCountAfterAuthentication(ID0, newCount));
  EXPECT_TRUE(OTRadioLink::SimpleSecureFrame32or0BodyRXV0p2::getInstance().getLastRXMessageCounter(ID0, mcbuf));
  EXPECT_EQ(0, memcmp(mcbuf, newCount, sizeof(mcbuf)));
  // Attempting to update to a lower (0) value should fail.
  EXPECT_TRUE(!OTRadioLink::SimpleSecureFrame32or0BodyRXV0p2::getInstance().updateRXMessageCountAfterAuthentication(ID0, zeroBlock));
  // Add/test second association...
  const uint8_t ID1[] = { 0x88, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0x8f };
  EXPECT_EQ(1, OTV0P2BASE::addNodeAssociation(ID1));
  EXPECT_EQ(2, OTV0P2BASE::countNodeAssociations());
  // Zero-length-lookup matches any entry.
  EXPECT_EQ(0, OTV0P2BASE::getNextMatchingNodeID(0, NULL, 0, NULL));
  EXPECT_EQ(1, OTV0P2BASE::getNextMatchingNodeID(1, NULL, 0, NULL));
  for(uint8_t i = 1; i <= sizeof(ID1); ++i)
    { EXPECT_EQ(1, OTV0P2BASE::getNextMatchingNodeID(0, ID1, i, NULL)); }
  for(uint8_t i = 1; i <= sizeof(ID1); ++i)
    { EXPECT_EQ(1, OTV0P2BASE::getNextMatchingNodeID(1, ID1, i, NULL)); }
  // Test that first ID cannot be matched from after its index in the table.
  for(uint8_t i = 1; i <= sizeof(ID0); ++i)
    { EXPECT_EQ(0, OTV0P2BASE::getNextMatchingNodeID(0, ID0, i, NULL)); }
  for(uint8_t i = 1; i <= sizeof(ID0); ++i)
    { EXPECT_EQ(-1, OTV0P2BASE::getNextMatchingNodeID(1, ID0, i, NULL)); }
  // Updating (ID0) to a new (up-by-one) count and reading back should work.
  const uint8_t newCount2[] = { 0, 1, 2, 3, 4, 6 };
  EXPECT_TRUE(OTRadioLink::SimpleSecureFrame32or0BodyRXV0p2::getInstance().updateRXMessageCountAfterAuthentication(ID0, newCount2));
  EXPECT_TRUE(OTRadioLink::SimpleSecureFrame32or0BodyRXV0p2::getInstance().getLastRXMessageCounter(ID0, mcbuf));
  EXPECT_EQ(0, memcmp(mcbuf, newCount2, sizeof(mcbuf)));
  // Updating to a new (up-by-slightly-more-than-one) count and reading back should work.
  const uint8_t newCount3[] = { 0, 1, 2, 3, 4, 9 };
  EXPECT_TRUE(OTRadioLink::SimpleSecureFrame32or0BodyRXV0p2::getInstance().updateRXMessageCountAfterAuthentication(ID0, newCount3));
  EXPECT_TRUE(OTRadioLink::SimpleSecureFrame32or0BodyRXV0p2::getInstance().getLastRXMessageCounter(ID0, mcbuf));
  EXPECT_EQ(0, memcmp(mcbuf, newCount3, sizeof(mcbuf)));
  // Updating to a new (up-by-much-more-than-one) count and reading back should work.
  const uint8_t newCount4[] = { 0, 1, 2, 3, 4, 99 };
  EXPECT_TRUE(OTRadioLink::SimpleSecureFrame32or0BodyRXV0p2::getInstance().updateRXMessageCountAfterAuthentication(ID0, newCount4));
  EXPECT_TRUE(OTRadioLink::SimpleSecureFrame32or0BodyRXV0p2::getInstance().getLastRXMessageCounter(ID0, mcbuf));
  EXPECT_EQ(0, memcmp(mcbuf, newCount4, sizeof(mcbuf)));
  // Updating to a new (up-by-much-much-more-than-one) count and reading back should work.
  const uint8_t newCount5[] = { 0, 1, 0x99, 1, 0x81, (OTV0P2BASE::randRNG8() & 0x7f) };
  EXPECT_TRUE(OTRadioLink::SimpleSecureFrame32or0BodyRXV0p2::getInstance().updateRXMessageCountAfterAuthentication(ID0, newCount5));
  EXPECT_TRUE(OTRadioLink::SimpleSecureFrame32or0BodyRXV0p2::getInstance().getLastRXMessageCounter(ID0, mcbuf));
  EXPECT_EQ(0, memcmp(mcbuf, newCount5, sizeof(mcbuf)));
  // Updating to a new (up-by-one) count and reading back should work.
  const uint8_t newCount6[] = { 0, 1, 0x99, 1, 0x81, 1+newCount5[5] };
  EXPECT_TRUE(OTRadioLink::SimpleSecureFrame32or0BodyRXV0p2::getInstance().updateRXMessageCountAfterAuthentication(ID0, newCount6));
  EXPECT_TRUE(OTRadioLink::SimpleSecureFrame32or0BodyRXV0p2::getInstance().getLastRXMessageCounter(ID0, mcbuf));
  EXPECT_EQ(0, memcmp(mcbuf, newCount6, sizeof(mcbuf)));
  // Updating to a new (up-by-one) count and reading back should work.
  const uint8_t newCount7[] = { 0, 1, 0x99, 1, 0x81, 2+newCount5[5] };
  EXPECT_TRUE(OTRadioLink::SimpleSecureFrame32or0BodyRXV0p2::getInstance().updateRXMessageCountAfterAuthentication(ID0, newCount7));
  EXPECT_TRUE(OTRadioLink::SimpleSecureFrame32or0BodyRXV0p2::getInstance().getLastRXMessageCounter(ID0, mcbuf));
  EXPECT_EQ(0, memcmp(mcbuf, newCount7, sizeof(mcbuf)));
  // Updating to a new (up-by-one) count and reading back should work.
  const uint8_t newCount8[] = { 0, 1, 0x99, 1, 0x81, 3+newCount5[5] };
  EXPECT_TRUE(OTRadioLink::SimpleSecureFrame32or0BodyRXV0p2::getInstance().updateRXMessageCountAfterAuthentication(ID0, newCount8));
  EXPECT_TRUE(OTRadioLink::SimpleSecureFrame32or0BodyRXV0p2::getInstance().getLastRXMessageCounter(ID0, mcbuf));
  EXPECT_EQ(0, memcmp(mcbuf, newCount8, sizeof(mcbuf)));
  // Updating to a new (up-by-one) count and reading back should work.
  const uint8_t newCount9[] = { 0, 1, 0x99, 1, 0x81, 4+newCount5[5] };
  EXPECT_TRUE(OTRadioLink::SimpleSecureFrame32or0BodyRXV0p2::getInstance().updateRXMessageCountAfterAuthentication(ID0, newCount9));
  EXPECT_TRUE(OTRadioLink::SimpleSecureFrame32or0BodyRXV0p2::getInstance().getLastRXMessageCounter(ID0, mcbuf));
  EXPECT_EQ(0, memcmp(mcbuf, newCount9, sizeof(mcbuf)));
  }
#endif

// Mock TX base: all zeros fixed IV and counters, valid fixed ID.
class TXBaseMock final : public OTRadioLink::SimpleSecureFrame32or0BodyTXBase
  {
  public:
    // Get TX ID that will be used for transmission; returns false on failure.
    // Argument must be buffer of (at least) OTV0P2BASE::OpenTRV_Node_ID_Bytes bytes.
    virtual bool getTXID(uint8_t *id) const override { memset(id, 0x80, OTV0P2BASE::OpenTRV_Node_ID_Bytes); return(true); }
    // Get the 3 bytes of persistent reboot/restart message counter, ie 3 MSBs of message counter; returns false on failure.
    virtual bool getTXNVCtrPrefix(uint8_t *buf) const override { memset(buf, 0, 3); return(true); }
    // Reset the persistent reboot/restart message counter; returns false on failure.
    virtual bool resetTXNVCtrPrefix(bool /*allZeros*/ = false) override { return(false); }
    // Increment persistent reboot/restart message counter; returns false on failure.
    virtual bool incrementTXNVCtrPrefix() override { return(false); }
    // Fills the supplied 6-byte array with the incremented monotonically-increasing primary TX counter.
    virtual bool getNextTXMsgCtr(uint8_t *buf) override { memset(buf, 0, 6); return(true); }
  };

// Test encoding of O frames through to final byte pattern.
TEST(OTAESGCMSecureFrame, OFrameEncoding)
{
    TXBaseMock mockTX;

    // All zeroes key.
    const uint8_t *const key = zeroBlock;
    // Size of buffer to receive encrypted frame.
    constexpr uint8_t encBufSize = 64;
    // Length of ID prefix for frame.
    const uint8_t txIDLen =4;
    // Distinguished 'invalid' valve position; never mistaken for a real valve.
    constexpr uint8_t valvePC = 0x7f;

    // Expected result.
    const uint8_t expected[63] = {62,207,4,128,128,128,128,32,102,58,109,143,127,209,106,16,122,170,41,17,135,168,193,220,188,110,36,204,190,21,125,138,196,172,122,155,149,87,43,4,0,0,0,0,0,0,162,222,15,42,215,77,210,0,127,19,255,121,139,199,19,12,128};

    // Encrypt empty (no-JSON) O frame via the explicit workspace API.
    uint8_t _bufW[encBufSize];
    OTRadioLink::OTBuf_t bufW(_bufW, sizeof(_bufW));

    uint8_t _rawFrame[34] = {};  // the length bbuf needs to be (valvepc + hasstats + msg including {} ).
    OTRadioLink::OTBuf_t rawFrame(_rawFrame, sizeof(_rawFrame));
    constexpr size_t workspaceSize = OTRadioLink::SimpleSecureFrame32or0BodyTXBase::encodeValveFrame_total_scratch_usage_OTAESGCM_2p0;
    uint8_t workspace[workspaceSize];
    OTV0P2BASE::ScratchSpaceL sW(workspace, workspaceSize);
    OTRadioLink::SimpleSecureFrame32or0BodyTXBase::fixed32BTextSize12BNonce16BTagSimpleEnc_fn_t &eW = OTAESGCM::fixed32BTextSize12BNonce16BTagSimpleEnc_DEFAULT_WITH_LWORKSPACE;

    OTRadioLink::OTEncodeData_T fd(_rawFrame, sizeof(_rawFrame), _bufW, sizeof(_bufW));
    const uint8_t bodylenW = mockTX.encodeValveFrame(
                                        fd,
                                        txIDLen,
                                        valvePC,
                                        eW,
                                        sW, key);
    
    EXPECT_EQ(63, bodylenW);
    for(int i = 0; i < bodylenW; ++i) { ASSERT_EQ(expected[i], bufW.buf[i]); }
}

// Test encoding of generic frames through to final byte pattern.
TEST(OTAESGCMSecureFrame, GenericFrameEncodingValidity)
{
    TXBaseMock mockTX;

    // All zeroes key.
    const uint8_t *const key = zeroBlock;
    // Size of buffer to receive encrypted frame.
    constexpr uint8_t encBufSize = 64;
    // Length of ID prefix for frame.
    const uint8_t txIDLen = 4;
    // // Distinguished 'invalid' valve position; never mistaken for a real valve.
    constexpr uint8_t valvePC = 0x7f;

    // Encrypt empty (no-JSON) O frame via the explicit workspace API.
    uint8_t _bufW[encBufSize] = {};
    OTRadioLink::OTBuf_t bufW(_bufW, sizeof(_bufW));

    uint8_t _rawFrame[34] = {};  // the length bbuf needs to be (valvepc + hasstats + msg including {} ).
    OTRadioLink::OTBuf_t rawFrame(_rawFrame, sizeof(_rawFrame));
    constexpr size_t workspaceSize = OTRadioLink::SimpleSecureFrame32or0BodyTXBase::encode_total_scratch_usage_OTAESGCM_2p0;
    uint8_t workspace[workspaceSize];
    OTV0P2BASE::ScratchSpaceL sW(workspace, workspaceSize);
    OTRadioLink::SimpleSecureFrame32or0BodyTXBase::fixed32BTextSize12BNonce16BTagSimpleEnc_fn_t &eW = OTAESGCM::fixed32BTextSize12BNonce16BTagSimpleEnc_DEFAULT_WITH_LWORKSPACE;

    // Setup frame data.
    OTRadioLink::OTEncodeData_T fd(_rawFrame, sizeof(_rawFrame), _bufW, sizeof(_bufW));
    { // This block is lifted and simplified from encodeValveFrame to build an identical frame.
        fd.ptext[0] = valvePC;
        fd.ptext[1] = 0; // Indicate presence of stats.
        fd.ptextLen = 2; // Note: callee will pad beyond this.
    }

    // // Test a too small scratchspace
    fd.fType = OTRadioLink::FTS_BasicSensorOrValve;
    OTV0P2BASE::ScratchSpaceL nullSW(workspace, 0);
    EXPECT_EQ(0, mockTX.encode(fd, txIDLen, eW, nullSW, key));
    for(size_t i = 0; i < sizeof(_bufW); ++i) { ASSERT_EQ(0, bufW.buf[i]); }
    OTV0P2BASE::ScratchSpaceL smallSW(workspace, workspaceSize - 1);
    EXPECT_EQ(0, mockTX.encode(fd, txIDLen, eW, smallSW, key));
    for(size_t i = 0; i < sizeof(_bufW); ++i) { ASSERT_EQ(0, bufW.buf[i]); }

    // // Test a few invalid fType values
    fd.fType = OTRadioLink::FTS_NONE;
    EXPECT_EQ(0, mockTX.encode(fd, txIDLen, eW, sW, key));
    for(size_t i = 0; i < sizeof(_bufW); ++i) { ASSERT_EQ(0, bufW.buf[i]); }
    fd.fType = OTRadioLink::FTS_INVALID_HIGH;
    EXPECT_EQ(0, mockTX.encode(fd, txIDLen, eW, sW, key));
    for(size_t i = 0; i < sizeof(_bufW); ++i) { ASSERT_EQ(0, bufW.buf[i]); }
    fd.fType = OTRadioLink::FTS_INVALID_HIGH;
    EXPECT_EQ(0, mockTX.encode(fd, txIDLen, eW, sW, key));
    for(size_t i = 0; i < sizeof(_bufW); ++i) { ASSERT_EQ(0, bufW.buf[i]); }
    fd.fType = (OTRadioLink::FrameType_Secureable)255;
    EXPECT_EQ(0, mockTX.encode(fd, txIDLen, eW, sW, key));
    for(size_t i = 0; i < sizeof(_bufW); ++i) { ASSERT_EQ(0, bufW.buf[i]); }

    // Test invalid il lengths.
    fd.fType = OTRadioLink::FTS_BasicSensorOrValve;
    EXPECT_EQ(0, mockTX.encode(fd, 255, eW, sW, key));
    for(size_t i = 0; i < sizeof(_bufW); ++i) { ASSERT_EQ(0, bufW.buf[i]); }
    EXPECT_EQ(0, mockTX.encode(fd, 6, eW, sW, key));
    for(size_t i = 0; i < sizeof(_bufW); ++i) { ASSERT_EQ(0, bufW.buf[i]); }

    // Test valid il lengths
    EXPECT_EQ(59, mockTX.encode(fd, 0, eW, sW, key));
    EXPECT_EQ(64, mockTX.encode(fd, 5, eW, sW, key));
    
    // Test il_ > 5
    // Frame length must be 0 for IDs longer than 5 bytes or it won't fit in frame.
    fd.ptextLen = 0;
    EXPECT_EQ(32, mockTX.encode(fd, 5, eW, sW, key));
    EXPECT_EQ(33, mockTX.encode(fd, 6, eW, sW, key));
    EXPECT_EQ(34, mockTX.encode(fd, 7, eW, sW, key));
    EXPECT_EQ(35, mockTX.encode(fd, 8, eW, sW, key));
    EXPECT_EQ(0, mockTX.encode(fd, 9, eW, sW, key));
}

// Test encoding of generic frames through to final byte pattern.
TEST(OTAESGCMSecureFrame, GenericFrameEncoding)
{
    TXBaseMock mockTX;

    // All zeroes key.
    const uint8_t *const key = zeroBlock;
    // Size of buffer to receive encrypted frame.
    constexpr uint8_t encBufSize = 64;
    // Length of ID prefix for frame.
    const uint8_t txIDLen =4;
    // // Distinguished 'invalid' valve position; never mistaken for a real valve.
    constexpr uint8_t valvePC = 0x7f;

    // Expected result.
    const uint8_t expected[63] = {62,207,4,128,128,128,128,32,102,58,109,143,127,209,106,16,122,170,41,17,135,168,193,220,188,110,36,204,190,21,125,138,196,172,122,155,149,87,43,4,0,0,0,0,0,0,162,222,15,42,215,77,210,0,127,19,255,121,139,199,19,12,128};

    // Encrypt empty (no-JSON) O frame via the explicit workspace API.
    uint8_t _bufW[encBufSize];
    OTRadioLink::OTBuf_t bufW(_bufW, sizeof(_bufW));

    uint8_t _rawFrame[34] = {};  // the length bbuf needs to be (valvepc + hasstats + msg including {} ).
    OTRadioLink::OTBuf_t rawFrame(_rawFrame, sizeof(_rawFrame));
    constexpr size_t workspaceSize = OTRadioLink::SimpleSecureFrame32or0BodyTXBase::encode_total_scratch_usage_OTAESGCM_2p0;
    uint8_t workspace[workspaceSize];
    OTV0P2BASE::ScratchSpaceL sW(workspace, workspaceSize);
    OTRadioLink::SimpleSecureFrame32or0BodyTXBase::fixed32BTextSize12BNonce16BTagSimpleEnc_fn_t &eW = OTAESGCM::fixed32BTextSize12BNonce16BTagSimpleEnc_DEFAULT_WITH_LWORKSPACE;

    // Setup frame data.
    OTRadioLink::OTEncodeData_T fd(_rawFrame, sizeof(_rawFrame), _bufW, sizeof(_bufW));
    { // This block is lifted and simplified from encodeValveFrame to build an identical frame.
        const char *const statsJSON = (const char *)&fd.ptext[2];
        const bool hasStats = (NULL != fd.ptext) && ('{' == statsJSON[0]);
        const size_t slp1 = hasStats ? strlen(statsJSON) : 1; // Stats length including trailing '}' (not sent).
        const uint8_t statslen = (uint8_t)(slp1 - 1); // Drop trailing '}' implicitly.
        fd.ptext[0] = valvePC;
        fd.ptext[1] = hasStats ? 0x10 : 0; // Indicate presence of stats.
        fd.ptextLen = (hasStats ? 2+statslen : 2); // Note: callee will pad beyond this.
    }
    fd.fType = OTRadioLink::FTS_BasicSensorOrValve;

    const uint8_t bodylenW = mockTX.encode(fd, txIDLen, eW, sW, key);
    EXPECT_EQ(63, bodylenW);
    for(int i = 0; i < bodylenW; ++i) { ASSERT_EQ(expected[i], bufW.buf[i]); }
}

// Encode section of GCMVS1ViaFixed32BTextSize test, measuring stack usage.
TEST(OTAESGCMSecureFrame, SecureFrameEncodeStackUsageWithWorkspace) {
    // Set up stack usage checks
    OTV0P2BASE::RAMEND = OTV0P2BASE::getSP();
    OTV0P2BASE::MemoryChecks::resetMinSP();
    OTV0P2BASE::MemoryChecks::recordIfMinSP();
    const size_t baseStack = OTV0P2BASE::MemoryChecks::getMinSP();



    EXPECT_GT(maxStackSecureFrameEncode, baseStack - OTV0P2BASE::MemoryChecks::getMinSP());
}

// Encode section of GCMVS1ViaFixed32BTextSize test, measuring stack usage.
TEST(OTAESGCMSecureFrame, SecureFrameDecodeStackUsage) {
    // Set up stack usage checks
    OTV0P2BASE::RAMEND = OTV0P2BASE::getSP();
    OTV0P2BASE::MemoryChecks::resetMinSP();
    OTV0P2BASE::MemoryChecks::recordIfMinSP();
    const size_t baseStack = OTV0P2BASE::MemoryChecks::getMinSP();

    // workspaces
    constexpr size_t encWorkspaceSize = OTRadioLink::SimpleSecureFrame32or0BodyTXBase::encodeRaw_total_scratch_usage_OTAESGCM_2p0;
    EXPECT_EQ(272U, encWorkspaceSize);
    uint8_t encWorkspace[encWorkspaceSize];
    OTV0P2BASE::ScratchSpaceL sWEnc(encWorkspace, sizeof(encWorkspace));
    constexpr size_t decWorkspaceSize =
            OTRadioLink::SimpleSecureFrame32or0BodyRXBase::decodeRaw_total_scratch_usage_OTAESGCM_3p0
            + OTAESGCM::OTAES128GCMGenericWithWorkspace<>::workspaceRequiredDec;
    EXPECT_EQ(320U, decWorkspaceSize);
    uint8_t decWorkspace[decWorkspaceSize];
    OTV0P2BASE::ScratchSpaceL sWDec(decWorkspace, sizeof(decWorkspace));


    uint8_t _buf[OTRadioLink::SecurableFrameHeader::maxSmallFrameSize];
    OTRadioLink::OTBuf_t buf(_buf, sizeof(_buf));
    //Example 3: secure, no valve, representative minimum stats {"b":1}).
    //Note that the sequence number must match the 4 lsbs of the message count, ie from iv[11].
    //and the ID is 0xaa 0xaa 0xaa 0xaa (transmitted) with the next ID bytes 0x55 0x55.
    //ResetCounter = 42
    //TxMsgCounter = 793
    //(Thus nonce/IV: aa aa aa aa 55 55 00 00 2a 00 03 19)
    //
    //3e cf 94 aa aa aa aa 20 | b3 45 f9 29 69 57 0c b8 28 66 14 b4 f0 69 b0 08 71 da d8 fe 47 c1 c3 53 83 48 88 03 7d 58 75 75 | 00 00 2a 00 03 19 29 3b 31 52 c3 26 d2 6d d0 8d 70 1e 4b 68 0d cb 80
    //
    //3e  length of header (62) after length byte 5 + (encrypted) body 32 + trailer 32
    //cf  'O' secure OpenTRV basic frame
    //04  0 sequence number, ID length 4
    //aa  ID byte 1
    //aa  ID byte 2
    //aa  ID byte 3
    //aa  ID byte 4
    //20  body length 32 (after padding and encryption)
    //    Plaintext body (length 8): 0x7f 0x11 { " b " : 1
    //    Padded: 7f 11 7b 22 62 22 3a 31 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 17
    //b3 45 f9 ... 58 75 75  32 bytes of encrypted body
    //00 00 2a  reset counter
    //00 03 19  message counter
    //29 3b 31 ... 68 0d cb  16 bytes of authentication tag
    //80  enc/auth type/format indicator.
    // Preshared ID prefix; only an initial part/prefix of this goes on the wire in the header.
    uint8_t id[] = { 0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55 };
    OTRadioLink::OTBuf_t id4bytes(id, 4);
    // IV/nonce starting with first 6 bytes of preshared ID, then 6 bytes of counter.
    const uint8_t iv[] = { 0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x00, 0x00, 0x2a, 0x00, 0x03, 0x19 };
    // 'O' frame body with some JSON stats.
    const uint8_t body[] = { 0x7f, 0x11, 0x7b, 0x22, 0x62, 0x22, 0x3a, 0x31 };
    uint8_t _bodyBuf[32] = {};
    OTRadioLink::OTBuf_t bodyBuf(_bodyBuf, sizeof(_bodyBuf));
    memcpy(bodyBuf.buf, body, sizeof(body));

    OTRadioLink::OTEncodeData_T fdTX(_bodyBuf, sizeof(_bodyBuf), _buf, sizeof(_buf));
    fdTX.ptextLen = sizeof(body);
    fdTX.fType = OTRadioLink::FTS_BasicSensorOrValve;
    const uint8_t encodedLength = OTRadioLink::SimpleSecureFrame32or0BodyTXBase::encodeRaw(
                                        fdTX,
                                        id4bytes.buf,
                                        id4bytes.bufsize,
                                        iv,
                                        OTAESGCM::fixed32BTextSize12BNonce16BTagSimpleEnc_DEFAULT_WITH_LWORKSPACE,
                                        sWEnc, zeroBlock);
    EXPECT_EQ(63, encodedLength);

    // (Nominally a longer ID and key is looked up with the ID in the header, and an iv built.)
    uint8_t decryptedBodyOut[OTRadioLink::ENC_BODY_SMALL_FIXED_PTEXT_MAX_SIZE];
    // To decode, emulating RX, structurally validate unpack the header and extract the ID.
    OTRadioLink::OTDecodeData_T fdRX(buf.buf, decryptedBodyOut);
    EXPECT_TRUE(0 != fdRX.sfh.decodeHeader(buf.buf, encodedLength));
    // Should decode and authenticate correctly.
    EXPECT_TRUE(0 != OTRadioLink::SimpleSecureFrame32or0BodyRXBase::decodeRaw(
                        fdRX,
                        OTAESGCM::fixed32BTextSize12BNonce16BTagSimpleDec_DEFAULT_WITH_LWORKSPACE,
                        sWDec, zeroBlock, iv));
    // Body content should be correctly decrypted and extracted.
    EXPECT_EQ(sizeof(body), fdRX.ptextLen);
    EXPECT_EQ(0, memcmp(body, decryptedBodyOut, sizeof(body)));

    // Using ASSERT to avoid cryptic crash message (Floating point exception (core dumped)) when encodedLength is 0.
    ASSERT_NE(0, encodedLength);
    // Check that flipping any single bit should make the decode fail
    // unless it leaves all info (seqNum, id, body) untouched.
    const uint8_t loc = OTV0P2BASE::randRNG8() % encodedLength;
    const uint8_t mask = (uint8_t) (0x100U >> (1+(OTV0P2BASE::randRNG8()&7)));
    buf.buf[loc] ^= mask;
    //  Serial.println(loc);
    //  Serial.println(mask);
    EXPECT_TRUE(
            (0 == fdRX.sfh.decodeHeader(buf.buf, encodedLength)) ||
            (0 == OTRadioLink::SimpleSecureFrame32or0BodyRXBase::decodeRaw(fdRX,
                                        OTAESGCM::fixed32BTextSize12BNonce16BTagSimpleDec_DEFAULT_WITH_LWORKSPACE,
                                        sWDec, zeroBlock, iv)) ||
            ((sizeof(body) == fdRX.ptextLen) && (0 == memcmp(body, decryptedBodyOut, sizeof(body))) && (0 == memcmp(id, fdRX.sfh.id, 4)))
    );
    EXPECT_GT(maxStackSecureFrameDecode, baseStack - OTV0P2BASE::MemoryChecks::getMinSP());
}

#endif // ARDUINO_LIB_OTAESGCM
