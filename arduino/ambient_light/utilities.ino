/*
 * various utility functions
 * 
 * 25.4.2016 blaz.krizaj@gmail.com
 */

// finds last target character in string
int lastIndexOf(const char * string, char target)
{
   int ret = -1;
   int curIdx = 0;
   while(string[curIdx] != '\0')
   {
      if (string[curIdx] == target) ret = curIdx;
      curIdx++;
   }
   return ret;
}

// removes all ch from string
void remove(char* string, char ch)
{
  char* output = string;
  while (*string)
  {
    if (*string != ch)
    {
      *(output++) = *string;
    }
    ++string;
  }
  *output = 0;
}
