#include <assert.h>
#include <stdlib.h>

template<typename T, int Capacity>
struct fixed_stack
{
  T   Elements[Capacity];
  int Count;

  void
  Push(const T& NewElement)
  {
    assert(this->Count < this->Capacity);
    assert(0 <= this->Count);

    this->Elements[this->Count++] = NewElement;
  }

  T*
  Pop()
  {
    if(0 < this->Count)
    {
      --this->Count;
      return &this->Elements[this->Count];
    }
    return NULL;
  }

  T*
  Back()
  {
    if(0 < this->Count)
    {
      return &this->Elements[this->Count - 1];
    }
    return NULL;
  }

  int
  Clear()
  {
    int TempCount = this->Count;
    this->Count   = 0;
    return TempCount;
  }

  T& operator[](int Index)
  {
    assert(0 <= Index && Index < Count);
    return this->Elements[Index];
  }
};

template<typename T, int Capacity>
struct fixed_array
{
  T   Elements[Capacity];
  int Count;

  T*
  Append(const T& NewElement)
  {
    assert(this->Count < Capacity);
    assert(0 <= this->Count);

    this->Elements[this->Count++] = NewElement;
    return &this->Elements[this->Count - 1];
  }

  void
  Clear()
  {
    this->Count = 0;
  }

  void
  HardClear()
  {
    memset(this, 0, sizeof(*this));
  }

  int
  GetCount()
  {
    return this->Count;
  }

  int
  GetCapacity()
  {
    return Capacity;
  }

  T& operator[](int Index)
  {
    assert(0 <= Index && Index < Count);
    return this->Elements[Index];
  }
};

