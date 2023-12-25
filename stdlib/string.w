class string
{
  i8* data;
  i64 length;

  i64 size(string* this) { return this.length; }
}

extern i32 puts(i8* str);

i32 printString(string str)
{
  puts(str.data);
  return 0; // TODO remove return type when I add void
}