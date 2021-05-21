#-------------------------------------------------
#
# Project created by QtCreator 2019-04-18T02:17:47
#
#-------------------------------------------------

QT += core gui websockets networkauth

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

VERSION = 1.2.11
QMAKE_TARGET_COMPANY = ETM
QMAKE_TARGET_PRODUCT = UNM CLIENT
QMAKE_TARGET_DESCRIPTION = Proxy client for Warcraft III
QMAKE_TARGET_COPYRIGHT = motherfuunnick
win32:RC_ICONS = src/resources/unm.ico

CONFIG( debug, debug|release ) {
    TARGET = unmclientDebug
} else {
    TARGET = unmclient
}

TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.

DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

#QMAKE_CXXFLAGS += /bigobj
QMAKE_CXXFLAGS += /std:c++latest
QMAKE_CXXFLAGS_RELEASE += -O2
DESTDIR = ../../../

INCLUDEPATH += src
INCLUDEPATH += src/include

LIBS += -L$$PWD/src/lib -llibcurl_a
win32:LIBS += -L$$PWD/src/lib -lmpir
unix:LIBS += -L$$PWD/src/lib -lgmp

SOURCES += src/bncsutilinterface.cpp \
           src/base64.cpp \
           src/bncsutil/bncssha1.c \
           src/bncsutil/bsha1.cpp \
           src/bncsutil/cdkeydecoder.cpp \
           src/bncsutil/checkrevision.cpp \
           src/bncsutil/decodekey.cpp \
           src/bncsutil/file.cpp \
           src/bncsutil/libinfo.cpp \
           src/bncsutil/nls.c \
           src/bncsutil/oldauth.cpp \
           src/bncsutil/pe.c \
           src/bncsutil/stack.c \
           src/bnet.cpp \
           src/bnetbot.cpp \
           src/bnetprotocol.cpp \
           src/commandpacket.cpp \
           src/config.cpp \
           src/configsection.cpp \
           src/csvparser.cpp \
           src/customaspectratiopixmaplabel.cpp \
           src/customdynamicfontsizelabel.cpp \
           src/customstyledItemdelegate.cpp \
           src/customstyledItemdelegateirina.cpp \
           src/customtablewidgetitem.cpp \
           src/game.cpp \
           src/game_base.cpp \
           src/gameplayer.cpp \
           src/gameprotocol.cpp \
           src/gameslot.cpp \
           src/gpsprotocol.cpp \
           src/irinaws.cpp \
           src/language.cpp \
           src/map.cpp \
           src/packed.cpp \
           src/replay.cpp \
           src/savegame.cpp \
           src/socket.cpp \
           src/stormlib/FileStream.cpp \
           src/stormlib/SBaseCommon.cpp \
           src/stormlib/SBaseDumpData.cpp \
           src/stormlib/SBaseFileTable.cpp \
           src/stormlib/SBaseSubTypes.cpp \
           src/stormlib/SCompression.cpp \
           src/stormlib/SFileAddFile.cpp \
           src/stormlib/SFileAttributes.cpp \
           src/stormlib/SFileCompactArchive.cpp \
           src/stormlib/SFileCreateArchive.cpp \
           src/stormlib/SFileExtractFile.cpp \
           src/stormlib/SFileFindFile.cpp \
           src/stormlib/SFileGetFileInfo.cpp \
           src/stormlib/SFileListFile.cpp \
           src/stormlib/SFileOpenArchive.cpp \
           src/stormlib/SFileOpenFileEx.cpp \
           src/stormlib/SFilePatchArchives.cpp \
           src/stormlib/SFileReadFile.cpp \
           src/stormlib/SFileVerify.cpp \
           src/stormlib/adpcm/adpcm.cpp \
           src/stormlib/bzip2/blocksort.c \
           src/stormlib/bzip2/bzcompress.c \
           src/stormlib/bzip2/bzdecompress.c \
           src/stormlib/bzip2/bzlib.c \
           src/stormlib/bzip2/crctable.c \
           src/stormlib/bzip2/huffman.c \
           src/stormlib/bzip2/randtable.c \
           src/stormlib/huffman/huff.cpp \
           src/stormlib/jenkins/lookup3.c \
           src/stormlib/libtomcrypt/src/hashes/hash_memory.c \
           src/stormlib/libtomcrypt/src/hashes/md5.c \
           src/stormlib/libtomcrypt/src/hashes/sha1.c \
           src/stormlib/libtomcrypt/src/math/ltm_desc.c \
           src/stormlib/libtomcrypt/src/math/multi.c \
           src/stormlib/libtomcrypt/src/math/rand_prime.c \
           src/stormlib/libtomcrypt/src/misc/base64_decode.c \
           src/stormlib/libtomcrypt/src/misc/crypt_argchk.c \
           src/stormlib/libtomcrypt/src/misc/crypt_find_hash.c \
           src/stormlib/libtomcrypt/src/misc/crypt_find_prng.c \
           src/stormlib/libtomcrypt/src/misc/crypt_hash_descriptor.c \
           src/stormlib/libtomcrypt/src/misc/crypt_hash_is_valid.c \
           src/stormlib/libtomcrypt/src/misc/crypt_libc.c \
           src/stormlib/libtomcrypt/src/misc/crypt_ltc_mp_descriptor.c \
           src/stormlib/libtomcrypt/src/misc/crypt_prng_descriptor.c \
           src/stormlib/libtomcrypt/src/misc/crypt_prng_is_valid.c \
           src/stormlib/libtomcrypt/src/misc/crypt_register_hash.c \
           src/stormlib/libtomcrypt/src/misc/crypt_register_prng.c \
           src/stormlib/libtomcrypt/src/misc/zeromem.c \
           src/stormlib/libtomcrypt/src/pk/asn1/der_decode_bit_string.c \
           src/stormlib/libtomcrypt/src/pk/asn1/der_decode_boolean.c \
           src/stormlib/libtomcrypt/src/pk/asn1/der_decode_choice.c \
           src/stormlib/libtomcrypt/src/pk/asn1/der_decode_ia5_string.c \
           src/stormlib/libtomcrypt/src/pk/asn1/der_decode_integer.c \
           src/stormlib/libtomcrypt/src/pk/asn1/der_decode_object_identifier.c \
           src/stormlib/libtomcrypt/src/pk/asn1/der_decode_octet_string.c \
           src/stormlib/libtomcrypt/src/pk/asn1/der_decode_printable_string.c \
           src/stormlib/libtomcrypt/src/pk/asn1/der_decode_sequence_ex.c \
           src/stormlib/libtomcrypt/src/pk/asn1/der_decode_sequence_flexi.c \
           src/stormlib/libtomcrypt/src/pk/asn1/der_decode_sequence_multi.c \
           src/stormlib/libtomcrypt/src/pk/asn1/der_decode_short_integer.c \
           src/stormlib/libtomcrypt/src/pk/asn1/der_decode_utctime.c \
           src/stormlib/libtomcrypt/src/pk/asn1/der_decode_utf8_string.c \
           src/stormlib/libtomcrypt/src/pk/asn1/der_encode_bit_string.c \
           src/stormlib/libtomcrypt/src/pk/asn1/der_encode_boolean.c \
           src/stormlib/libtomcrypt/src/pk/asn1/der_encode_ia5_string.c \
           src/stormlib/libtomcrypt/src/pk/asn1/der_encode_integer.c \
           src/stormlib/libtomcrypt/src/pk/asn1/der_encode_object_identifier.c \
           src/stormlib/libtomcrypt/src/pk/asn1/der_encode_octet_string.c \
           src/stormlib/libtomcrypt/src/pk/asn1/der_encode_printable_string.c \
           src/stormlib/libtomcrypt/src/pk/asn1/der_encode_sequence_ex.c \
           src/stormlib/libtomcrypt/src/pk/asn1/der_encode_sequence_multi.c \
           src/stormlib/libtomcrypt/src/pk/asn1/der_encode_set.c \
           src/stormlib/libtomcrypt/src/pk/asn1/der_encode_setof.c \
           src/stormlib/libtomcrypt/src/pk/asn1/der_encode_short_integer.c \
           src/stormlib/libtomcrypt/src/pk/asn1/der_encode_utctime.c \
           src/stormlib/libtomcrypt/src/pk/asn1/der_encode_utf8_string.c \
           src/stormlib/libtomcrypt/src/pk/asn1/der_length_bit_string.c \
           src/stormlib/libtomcrypt/src/pk/asn1/der_length_boolean.c \
           src/stormlib/libtomcrypt/src/pk/asn1/der_length_ia5_string.c \
           src/stormlib/libtomcrypt/src/pk/asn1/der_length_integer.c \
           src/stormlib/libtomcrypt/src/pk/asn1/der_length_object_identifier.c \
           src/stormlib/libtomcrypt/src/pk/asn1/der_length_octet_string.c \
           src/stormlib/libtomcrypt/src/pk/asn1/der_length_printable_string.c \
           src/stormlib/libtomcrypt/src/pk/asn1/der_length_sequence.c \
           src/stormlib/libtomcrypt/src/pk/asn1/der_length_short_integer.c \
           src/stormlib/libtomcrypt/src/pk/asn1/der_length_utctime.c \
           src/stormlib/libtomcrypt/src/pk/asn1/der_length_utf8_string.c \
           src/stormlib/libtomcrypt/src/pk/asn1/der_sequence_free.c \
           src/stormlib/libtomcrypt/src/pk/ecc/ltc_ecc_map.c \
           src/stormlib/libtomcrypt/src/pk/ecc/ltc_ecc_mul2add.c \
           src/stormlib/libtomcrypt/src/pk/ecc/ltc_ecc_mulmod.c \
           src/stormlib/libtomcrypt/src/pk/ecc/ltc_ecc_points.c \
           src/stormlib/libtomcrypt/src/pk/ecc/ltc_ecc_projective_add_point.c \
           src/stormlib/libtomcrypt/src/pk/ecc/ltc_ecc_projective_dbl_point.c \
           src/stormlib/libtomcrypt/src/pk/pkcs1/pkcs_1_mgf1.c \
           src/stormlib/libtomcrypt/src/pk/pkcs1/pkcs_1_oaep_decode.c \
           src/stormlib/libtomcrypt/src/pk/pkcs1/pkcs_1_pss_decode.c \
           src/stormlib/libtomcrypt/src/pk/pkcs1/pkcs_1_pss_encode.c \
           src/stormlib/libtomcrypt/src/pk/pkcs1/pkcs_1_v1_5_decode.c \
           src/stormlib/libtomcrypt/src/pk/pkcs1/pkcs_1_v1_5_encode.c \
           src/stormlib/libtomcrypt/src/pk/rsa/rsa_exptmod.c \
           src/stormlib/libtomcrypt/src/pk/rsa/rsa_free.c \
           src/stormlib/libtomcrypt/src/pk/rsa/rsa_import.c \
           src/stormlib/libtomcrypt/src/pk/rsa/rsa_make_key.c \
           src/stormlib/libtomcrypt/src/pk/rsa/rsa_sign_hash.c \
           src/stormlib/libtomcrypt/src/pk/rsa/rsa_verify_hash.c \
           src/stormlib/libtomcrypt/src/pk/rsa/rsa_verify_simple.c \
           src/stormlib/libtommath/bn_fast_mp_invmod.c \
           src/stormlib/libtommath/bn_fast_mp_montgomery_reduce.c \
           src/stormlib/libtommath/bn_fast_s_mp_mul_digs.c \
           src/stormlib/libtommath/bn_fast_s_mp_mul_high_digs.c \
           src/stormlib/libtommath/bn_fast_s_mp_sqr.c \
           src/stormlib/libtommath/bn_mp_2expt.c \
           src/stormlib/libtommath/bn_mp_abs.c \
           src/stormlib/libtommath/bn_mp_add.c \
           src/stormlib/libtommath/bn_mp_add_d.c \
           src/stormlib/libtommath/bn_mp_addmod.c \
           src/stormlib/libtommath/bn_mp_and.c \
           src/stormlib/libtommath/bn_mp_clamp.c \
           src/stormlib/libtommath/bn_mp_clear.c \
           src/stormlib/libtommath/bn_mp_clear_multi.c \
           src/stormlib/libtommath/bn_mp_cmp.c \
           src/stormlib/libtommath/bn_mp_cmp_d.c \
           src/stormlib/libtommath/bn_mp_cmp_mag.c \
           src/stormlib/libtommath/bn_mp_cnt_lsb.c \
           src/stormlib/libtommath/bn_mp_copy.c \
           src/stormlib/libtommath/bn_mp_count_bits.c \
           src/stormlib/libtommath/bn_mp_div.c \
           src/stormlib/libtommath/bn_mp_div_2.c \
           src/stormlib/libtommath/bn_mp_div_2d.c \
           src/stormlib/libtommath/bn_mp_div_3.c \
           src/stormlib/libtommath/bn_mp_div_d.c \
           src/stormlib/libtommath/bn_mp_dr_is_modulus.c \
           src/stormlib/libtommath/bn_mp_dr_reduce.c \
           src/stormlib/libtommath/bn_mp_dr_setup.c \
           src/stormlib/libtommath/bn_mp_exch.c \
           src/stormlib/libtommath/bn_mp_expt_d.c \
           src/stormlib/libtommath/bn_mp_exptmod.c \
           src/stormlib/libtommath/bn_mp_exptmod_fast.c \
           src/stormlib/libtommath/bn_mp_exteuclid.c \
           src/stormlib/libtommath/bn_mp_fread.c \
           src/stormlib/libtommath/bn_mp_fwrite.c \
           src/stormlib/libtommath/bn_mp_gcd.c \
           src/stormlib/libtommath/bn_mp_get_int.c \
           src/stormlib/libtommath/bn_mp_grow.c \
           src/stormlib/libtommath/bn_mp_init.c \
           src/stormlib/libtommath/bn_mp_init_copy.c \
           src/stormlib/libtommath/bn_mp_init_multi.c \
           src/stormlib/libtommath/bn_mp_init_set.c \
           src/stormlib/libtommath/bn_mp_init_set_int.c \
           src/stormlib/libtommath/bn_mp_init_size.c \
           src/stormlib/libtommath/bn_mp_invmod.c \
           src/stormlib/libtommath/bn_mp_invmod_slow.c \
           src/stormlib/libtommath/bn_mp_is_square.c \
           src/stormlib/libtommath/bn_mp_jacobi.c \
           src/stormlib/libtommath/bn_mp_karatsuba_mul.c \
           src/stormlib/libtommath/bn_mp_karatsuba_sqr.c \
           src/stormlib/libtommath/bn_mp_lcm.c \
           src/stormlib/libtommath/bn_mp_lshd.c \
           src/stormlib/libtommath/bn_mp_mod.c \
           src/stormlib/libtommath/bn_mp_mod_2d.c \
           src/stormlib/libtommath/bn_mp_mod_d.c \
           src/stormlib/libtommath/bn_mp_montgomery_calc_normalization.c \
           src/stormlib/libtommath/bn_mp_montgomery_reduce.c \
           src/stormlib/libtommath/bn_mp_montgomery_setup.c \
           src/stormlib/libtommath/bn_mp_mul.c \
           src/stormlib/libtommath/bn_mp_mul_2.c \
           src/stormlib/libtommath/bn_mp_mul_2d.c \
           src/stormlib/libtommath/bn_mp_mul_d.c \
           src/stormlib/libtommath/bn_mp_mulmod.c \
           src/stormlib/libtommath/bn_mp_n_root.c \
           src/stormlib/libtommath/bn_mp_neg.c \
           src/stormlib/libtommath/bn_mp_or.c \
           src/stormlib/libtommath/bn_mp_prime_fermat.c \
           src/stormlib/libtommath/bn_mp_prime_is_divisible.c \
           src/stormlib/libtommath/bn_mp_prime_is_prime.c \
           src/stormlib/libtommath/bn_mp_prime_miller_rabin.c \
           src/stormlib/libtommath/bn_mp_prime_next_prime.c \
           src/stormlib/libtommath/bn_mp_prime_rabin_miller_trials.c \
           src/stormlib/libtommath/bn_mp_prime_random_ex.c \
           src/stormlib/libtommath/bn_mp_radix_size.c \
           src/stormlib/libtommath/bn_mp_radix_smap.c \
           src/stormlib/libtommath/bn_mp_rand.c \
           src/stormlib/libtommath/bn_mp_read_radix.c \
           src/stormlib/libtommath/bn_mp_read_signed_bin.c \
           src/stormlib/libtommath/bn_mp_read_unsigned_bin.c \
           src/stormlib/libtommath/bn_mp_reduce.c \
           src/stormlib/libtommath/bn_mp_reduce_2k.c \
           src/stormlib/libtommath/bn_mp_reduce_2k_l.c \
           src/stormlib/libtommath/bn_mp_reduce_2k_setup.c \
           src/stormlib/libtommath/bn_mp_reduce_2k_setup_l.c \
           src/stormlib/libtommath/bn_mp_reduce_is_2k.c \
           src/stormlib/libtommath/bn_mp_reduce_is_2k_l.c \
           src/stormlib/libtommath/bn_mp_reduce_setup.c \
           src/stormlib/libtommath/bn_mp_rshd.c \
           src/stormlib/libtommath/bn_mp_set.c \
           src/stormlib/libtommath/bn_mp_set_int.c \
           src/stormlib/libtommath/bn_mp_shrink.c \
           src/stormlib/libtommath/bn_mp_signed_bin_size.c \
           src/stormlib/libtommath/bn_mp_sqr.c \
           src/stormlib/libtommath/bn_mp_sqrmod.c \
           src/stormlib/libtommath/bn_mp_sqrt.c \
           src/stormlib/libtommath/bn_mp_sub.c \
           src/stormlib/libtommath/bn_mp_sub_d.c \
           src/stormlib/libtommath/bn_mp_submod.c \
           src/stormlib/libtommath/bn_mp_to_signed_bin.c \
           src/stormlib/libtommath/bn_mp_to_signed_bin_n.c \
           src/stormlib/libtommath/bn_mp_to_unsigned_bin.c \
           src/stormlib/libtommath/bn_mp_to_unsigned_bin_n.c \
           src/stormlib/libtommath/bn_mp_toom_mul.c \
           src/stormlib/libtommath/bn_mp_toom_sqr.c \
           src/stormlib/libtommath/bn_mp_toradix.c \
           src/stormlib/libtommath/bn_mp_toradix_n.c \
           src/stormlib/libtommath/bn_mp_unsigned_bin_size.c \
           src/stormlib/libtommath/bn_mp_xor.c \
           src/stormlib/libtommath/bn_mp_zero.c \
           src/stormlib/libtommath/bn_prime_tab.c \
           src/stormlib/libtommath/bn_reverse.c \
           src/stormlib/libtommath/bn_s_mp_add.c \
           src/stormlib/libtommath/bn_s_mp_exptmod.c \
           src/stormlib/libtommath/bn_s_mp_mul_digs.c \
           src/stormlib/libtommath/bn_s_mp_mul_high_digs.c \
           src/stormlib/libtommath/bn_s_mp_sqr.c \
           src/stormlib/libtommath/bn_s_mp_sub.c \
           src/stormlib/libtommath/bncore.c \
           src/stormlib/lzma/C/LzFind.c \
           src/stormlib/lzma/C/LzFindMt.c \
           src/stormlib/lzma/C/LzmaDec.c \
           src/stormlib/lzma/C/LzmaEnc.c \
           src/stormlib/lzma/C/Threads.c \
           src/stormlib/pklib/explode.c \
           src/stormlib/pklib/implode.c \
           src/stormlib/pklib/pkcrc32.c \
           src/stormlib/sparse/sparse.cpp \
           src/stormlib/zlib/adler32.c \
           src/stormlib/zlib/compress.c \
           src/stormlib/zlib/crc32.c \
           src/stormlib/zlib/deflate.c \
           src/stormlib/zlib/inffast.c \
           src/stormlib/zlib/inflate.c \
           src/stormlib/zlib/inftrees.c \
           src/stormlib/zlib/trees.c \
           src/stormlib/zlib/zutil.c \
           src/unm.cpp \
           src/unmcrc32.cpp \
           src/unmprotocol.cpp \
           src/unmsha1.cpp \
           src/util.cpp

HEADERS += src/bncsutilinterface.h \
           src/base64.h \
           src/bncsutil/bncssha1.h \
           src/bncsutil/bncsutil.h \
           src/bncsutil/bsha1.h \
           src/bncsutil/cdkeydecoder.h \
           src/bncsutil/checkrevision.h \
           src/bncsutil/decodekey.h \
           src/bncsutil/file.h \
           src/bncsutil/gmp.h \
           src/bncsutil/keytables.h \
           src/bncsutil/libinfo.h \
           src/bncsutil/mpir.h \
           src/bncsutil/ms_stdint.h \
           src/bncsutil/mutil.h \
           src/bncsutil/mutil_types.h \
           src/bncsutil/nls.h \
           src/bncsutil/oldauth.h \
           src/bncsutil/pe.h \
           src/bncsutil/stack.h \
           src/bnet.h \
           src/bnetbot.h \
           src/bnetprotocol.h \
           src/commandpacket.h \
           src/config.h \
           src/configsection.h \
           src/csvparser.h \
           src/customaspectratiopixmaplabel.h \
           src/customdynamicfontsizelabel.h \
           src/custommaintabbar.h \
           src/custommaintabwidget.h \
           src/customstyledItemdelegate.h \
           src/customstyledItemdelegateirina.h \
           src/customtabbar.h \
           src/customtablewidgetitem.h \
           src/customtabwidget.h \
           src/game.h \
           src/game_base.h \
           src/gameplayer.h \
           src/gameprotocol.h \
           src/gameslot.h \
           src/gpsprotocol.h \
           src/includes.h \
           src/irinaws.h \
           src/language.h \
           src/map.h \
           src/ms_stdint.h \
           src/mysql/my_list.h \
           src/mysql/mysql.h \
           src/mysql/mysql_com.h \
           src/mysql/mysql_time.h \
           src/mysql/mysql_version.h \
           src/mysql/typelib.h \
           src/next_combination.h \
           src/packed.h \
           src/replay.h \
           src/savegame.h \
           src/socket.h \
           src/stormlib/FileStream.h \
           src/stormlib/StormCommon.h \
           src/stormlib/StormLib.h \
           src/stormlib/StormPort.h \
           src/stormlib/adpcm/adpcm.h \
           src/stormlib/bzip2/bzlib.h \
           src/stormlib/bzip2/bzlib_private.h \
           src/stormlib/huffman/huff.h \
           src/stormlib/jenkins/lookup.h \
           src/stormlib/libtomcrypt/src/headers/tomcrypt.h \
           src/stormlib/libtomcrypt/src/headers/tomcrypt_argchk.h \
           src/stormlib/libtomcrypt/src/headers/tomcrypt_cfg.h \
           src/stormlib/libtomcrypt/src/headers/tomcrypt_cipher.h \
           src/stormlib/libtomcrypt/src/headers/tomcrypt_custom.h \
           src/stormlib/libtomcrypt/src/headers/tomcrypt_hash.h \
           src/stormlib/libtomcrypt/src/headers/tomcrypt_mac.h \
           src/stormlib/libtomcrypt/src/headers/tomcrypt_macros.h \
           src/stormlib/libtomcrypt/src/headers/tomcrypt_math.h \
           src/stormlib/libtomcrypt/src/headers/tomcrypt_misc.h \
           src/stormlib/libtomcrypt/src/headers/tomcrypt_pk.h \
           src/stormlib/libtomcrypt/src/headers/tomcrypt_pkcs.h \
           src/stormlib/libtomcrypt/src/headers/tomcrypt_prng.h \
           src/stormlib/libtommath/tommath.h \
           src/stormlib/libtommath/tommath_class.h \
           src/stormlib/libtommath/tommath_superclass.h \
           src/stormlib/lzma/C/LzFind.h \
           src/stormlib/lzma/C/LzFindMt.h \
           src/stormlib/lzma/C/LzHash.h \
           src/stormlib/lzma/C/LzmaDec.h \
           src/stormlib/lzma/C/LzmaEnc.h \
           src/stormlib/lzma/C/Threads.h \
           src/stormlib/lzma/C/Types.h \
           src/stormlib/pklib/pklib.h \
           src/stormlib/sparse/sparse.h \
           src/stormlib/zlib/crc32.h \
           src/stormlib/zlib/deflate.h \
           src/stormlib/zlib/inffast.h \
           src/stormlib/zlib/inffixed.h \
           src/stormlib/zlib/inflate.h \
           src/stormlib/zlib/inftrees.h \
           src/stormlib/zlib/trees.h \
           src/stormlib/zlib/zconf.h \
           src/stormlib/zlib/zlib.h \
           src/stormlib/zlib/zutil.h \
           src/unm.h \
           src/unmcrc32.h \
           src/unmprotocol.h \
           src/unmsha1.h \
           src/util.h

FORMS += src/unm.ui \
         src/anotherwindow.ui \
         src/chatwidget.ui \
         src/chatwidgetirina.ui \
         src/gamewidget.ui \
         src/presettingwindow.ui \
         src/stylesettingwindow.ui \
         src/helplogonwindow.ui \
         src/logsettingswindow.ui

# Default rules for deployment.

qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += src/resources/icons.qrc
