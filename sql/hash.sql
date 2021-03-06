SET TERM ^ ;

CREATE OR ALTER PACKAGE HASH
AS
BEGIN

  FUNCTION key0s(hash_ SMALLINT NOT NULL, value_ VARCHAR(32765) CHARACTER SET NONE
    ) RETURNS VARCHAR(128) CHARACTER SET NONE;

  FUNCTION key0b(hash_ SMALLINT NOT NULL, value_ BLOB SUB_TYPE BINARY
    ) RETURNS VARCHAR(128) CHARACTER SET NONE;

  /*
  -- Siphash
  FUNCTION key2s(hash_ SMALLINT NOT NULL, custom VARCHAR(32765) CHARACTER SET NONE, value_ VARCHAR(32765) CHARACTER SET NONE
    ) RETURNS VARCHAR(128) CHARACTER SET NONE;

  FUNCTION key2b(hash_ SMALLINT NOT NULL, custom VARCHAR(32765) CHARACTER SET NONE, value_ BLOB SUB_TYPE BINARY
    ) RETURNS VARCHAR(128) CHARACTER SET NONE;

  -- Cshake_..., Shake_..., Tuple_hash_...
  FUNCTION key3s(hash_ SMALLINT NOT NULL, digest INTEGER NOT NULL, custom VARCHAR(32765) CHARACTER SET NONE, value_ VARCHAR(32765) CHARACTER SET NONE
    ) RETURNS VARCHAR(1024) CHARACTER SET NONE;

  FUNCTION key3b(hash_ SMALLINT NOT NULL, digest INTEGER NOT NULL, custom VARCHAR(32765) CHARACTER SET NONE, value_ BLOB SUB_TYPE BINARY
    ) RETURNS VARCHAR(1024) CHARACTER SET NONE;
  */

  FUNCTION Blake1_224 RETURNS SMALLINT;
  FUNCTION Blake1_256 RETURNS SMALLINT;
  FUNCTION Blake1_384 RETURNS SMALLINT;
  FUNCTION Blake1_512 RETURNS SMALLINT;
  FUNCTION Blake2 RETURNS SMALLINT;
  FUNCTION Blake2s RETURNS SMALLINT;
  FUNCTION Crc_32 RETURNS SMALLINT;
  -- FUNCTION Cshake_128 RETURNS SMALLINT; FUNCTION Cshake_256 RETURNS SMALLINT; /* will in future */
  FUNCTION Fnv32_1a RETURNS SMALLINT;
  FUNCTION Fnv64_1a RETURNS SMALLINT;
  FUNCTION Has160 RETURNS SMALLINT;
  FUNCTION Md2 RETURNS SMALLINT;
  FUNCTION Md4 RETURNS SMALLINT;
  FUNCTION Md5 RETURNS SMALLINT;
  FUNCTION Ripemd_128 RETURNS SMALLINT;
  FUNCTION Ripemd_160 RETURNS SMALLINT;
  FUNCTION Ripemd_256 RETURNS SMALLINT;
  FUNCTION Ripemd_320 RETURNS SMALLINT;
  FUNCTION Siphash RETURNS SMALLINT; -- key = { 0 } (keep it simple for now)
  FUNCTION Sha1 RETURNS SMALLINT;
  FUNCTION Sha2_224 RETURNS SMALLINT;
  FUNCTION Sha2_256 RETURNS SMALLINT;
  FUNCTION Sha2_384 RETURNS SMALLINT;
  FUNCTION Sha2_512 RETURNS SMALLINT;
  FUNCTION Sha2_512_224 RETURNS SMALLINT;
  FUNCTION Sha2_512_256 RETURNS SMALLINT;
  FUNCTION Sha3_224 RETURNS SMALLINT;
  FUNCTION Sha3_256 RETURNS SMALLINT;
  FUNCTION Sha3_384 RETURNS SMALLINT;
  FUNCTION Sha3_512 RETURNS SMALLINT;
  -- FUNCTION Shake_128 RETURNS SMALLINT; FUNCTION Shake_256 RETURNS SMALLINT; /* will in future */
  FUNCTION Sm3 RETURNS SMALLINT;
  FUNCTION Tiger1_128 RETURNS SMALLINT;
  FUNCTION Tiger1_160 RETURNS SMALLINT;
  FUNCTION Tiger1_192 RETURNS SMALLINT;
  FUNCTION Tiger2_128 RETURNS SMALLINT;
  FUNCTION Tiger2_160 RETURNS SMALLINT;
  FUNCTION Tiger2_192 RETURNS SMALLINT;
  -- FUNCTION Tuple_hash_128 RETURNS SMALLINT; FUNCTION Tuple_hash_256 RETURNS SMALLINT; /* will in future */
  FUNCTION Whirlpool RETURNS SMALLINT;

END^

RECREATE PACKAGE BODY HASH
AS
BEGIN

  FUNCTION key0s(hash_ SMALLINT NOT NULL,  value_ VARCHAR(32765) CHARACTER SET NONE
    ) RETURNS VARCHAR(128) CHARACTER SET NONE
    EXTERNAL NAME 'hash!key'
    ENGINE UDR;

  FUNCTION key0b(hash_ SMALLINT NOT NULL, value_ BLOB SUB_TYPE BINARY
    ) RETURNS VARCHAR(128) CHARACTER SET NONE
    EXTERNAL NAME 'hash!key'
    ENGINE UDR;

  FUNCTION Blake1_224 RETURNS SMALLINT AS BEGIN RETURN 1; END
  FUNCTION Blake1_256 RETURNS SMALLINT AS BEGIN RETURN 2; END
  FUNCTION Blake1_384 RETURNS SMALLINT AS BEGIN RETURN 3; END
  FUNCTION Blake1_512 RETURNS SMALLINT AS BEGIN RETURN 4; END
  FUNCTION Blake2 RETURNS SMALLINT AS BEGIN RETURN 5; END
  FUNCTION Blake2s RETURNS SMALLINT AS BEGIN RETURN 6; END
  FUNCTION Crc_32 RETURNS SMALLINT AS BEGIN RETURN 7; END
  FUNCTION Cshake_128 RETURNS SMALLINT AS BEGIN RETURN 8; END
  FUNCTION Cshake_256 RETURNS SMALLINT AS BEGIN RETURN 9; END
  FUNCTION Fnv32_1a RETURNS SMALLINT AS BEGIN RETURN 10; END
  FUNCTION Fnv64_1a RETURNS SMALLINT AS BEGIN RETURN 11; END
  FUNCTION Has160 RETURNS SMALLINT AS BEGIN RETURN 12; END
  FUNCTION Md2 RETURNS SMALLINT AS BEGIN RETURN 13; END
  FUNCTION Md4 RETURNS SMALLINT AS BEGIN RETURN 14; END
  FUNCTION Md5 RETURNS SMALLINT AS BEGIN RETURN 15; END
  FUNCTION Ripemd_128 RETURNS SMALLINT AS BEGIN RETURN 16; END
  FUNCTION Ripemd_160 RETURNS SMALLINT AS BEGIN RETURN 17; END
  FUNCTION Ripemd_256 RETURNS SMALLINT AS BEGIN RETURN 18; END
  FUNCTION Ripemd_320 RETURNS SMALLINT AS BEGIN RETURN 19; END
  FUNCTION Siphash RETURNS SMALLINT AS BEGIN RETURN 20; END
  FUNCTION Sha1 RETURNS SMALLINT AS BEGIN RETURN 21; END
  FUNCTION Sha2_224 RETURNS SMALLINT AS BEGIN RETURN 22; END
  FUNCTION Sha2_256 RETURNS SMALLINT AS BEGIN RETURN 23; END
  FUNCTION Sha2_384 RETURNS SMALLINT AS BEGIN RETURN 24; END
  FUNCTION Sha2_512 RETURNS SMALLINT AS BEGIN RETURN 25; END
  FUNCTION Sha2_512_224 RETURNS SMALLINT AS BEGIN RETURN 26; END
  FUNCTION Sha2_512_256 RETURNS SMALLINT AS BEGIN RETURN 27; END
  FUNCTION Sha3_224 RETURNS SMALLINT AS BEGIN RETURN 28; END
  FUNCTION Sha3_256 RETURNS SMALLINT AS BEGIN RETURN 29; END
  FUNCTION Sha3_384 RETURNS SMALLINT AS BEGIN RETURN 30; END
  FUNCTION Sha3_512 RETURNS SMALLINT AS BEGIN RETURN 31; END
  FUNCTION Shake_128 RETURNS SMALLINT AS BEGIN RETURN 32; END
  FUNCTION Shake_256 RETURNS SMALLINT AS BEGIN RETURN 33; END
  FUNCTION Sm3 RETURNS SMALLINT AS BEGIN RETURN 34; END
  FUNCTION Tiger1_128 RETURNS SMALLINT AS BEGIN RETURN 35; END
  FUNCTION Tiger1_160 RETURNS SMALLINT AS BEGIN RETURN 36; END
  FUNCTION Tiger1_192 RETURNS SMALLINT AS BEGIN RETURN 37; END
  FUNCTION Tiger2_128 RETURNS SMALLINT AS BEGIN RETURN 38; END
  FUNCTION Tiger2_160 RETURNS SMALLINT AS BEGIN RETURN 39; END
  FUNCTION Tiger2_192 RETURNS SMALLINT AS BEGIN RETURN 40; END
  FUNCTION Tuple_hash_128 RETURNS SMALLINT AS BEGIN RETURN 41; END
  FUNCTION Tuple_hash_256 RETURNS SMALLINT AS BEGIN RETURN 42; END
  FUNCTION Whirlpool RETURNS SMALLINT AS BEGIN RETURN 43; END

END^

SET TERM ; ^

/* Existing privileges on this package */

GRANT EXECUTE ON PACKAGE HASH TO SYSDBA;
