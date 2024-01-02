class string
{
  i8* data = null;
  i64 length = 0i64;

  i64 size(string* this) { return this.length; }
}

extern i32 puts(i8* str);

i32 printString(string str)
{
  puts(str.data);
  return 0; // TODO remove return type when I add void
}