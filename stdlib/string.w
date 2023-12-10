class string
{
  i8* data;
  i64 length;
}

extern i32 puts(i8* str);

i32 printString(string str)
{
  puts(str.data);
}