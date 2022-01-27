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
  hash_ = hash.string(hash.Whirlpool(), 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789');
  SUSPEND;
  hash_ = hash.blob_(hash.Whirlpool(), 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789');
  SUSPEND;
END
```
