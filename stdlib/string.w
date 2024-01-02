class string
{
  i8* data = null;
  i64 length = 0i64;
  i64 capacity = 0i64;

  i64 size(string* this) { return this.length; }
  i32 append(string* this, string* other)
  {
    // TODO: check capacity and don't reallocate if not needed
    i64 newLength = this.length + other.length;
    this.reallocate(newLength);
    memcpy(&this.data[this.length], other.data, other.length);
    this.length = this.length + other.length;
    this.data[this.length] = 0i8;
  }

  i32 reallocate(string* this, i64 newCapacity)
  {
    // TODO: assert newCapacity is big enough
    if (this.capacity == -1i64)
    {
      abort();
    }

    i8* temp = this.data;

    this.data = malloc(newCapacity);

    if (temp != null)
    {
      memcpy(this.data, temp, this.length);
      free(temp);
    }

    this.data[this.length] = 0i8;
    this.capacity = newCapacity;
  }
}

i32 print(string* str)
{
  puts(str.data);
  return 0; // TODO remove return type when I add void
}