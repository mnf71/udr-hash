# udr-hash
Firebird user defined routine based on easy-to-use hash library https://github.com/Chocobo1/Hash

Usage:

```sql
EXECUTE BLOCK
RETURNS (
  hash_ VARCHAR(128)
)
AS
BEGIN
  hash_ = hash.key0s(hash.Whirlpool(), 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789');
  SUSPEND;
  hash_ = hash.key0b(hash.Whirlpool(), 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789');
  SUSPEND;
END
```
