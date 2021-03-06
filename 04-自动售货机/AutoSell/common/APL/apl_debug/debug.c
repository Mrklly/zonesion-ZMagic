#include "debug.h"

#ifdef __GNUC__
/* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

static void (*put_ch)(char) = NULL;
process_event_t serial_line_event_message;

PUTCHAR_PROTOTYPE
{
  if (put_ch != NULL)
  {
    put_ch(ch);
  }
  return ch;
}

void debug_init(void)
{
  uart1_init(115200);
  uart1_set_input(debug_input);
  put_ch = uart1_putc;

  serial_line_event_message = process_alloc_event();
}


#if 1
PROCESS_NAME(serial_shell_process);

int debug_input(char c)
{
  static char buf[128];
  static int idx = 0;

  if (put_ch != NULL)
  {
    put_ch(c);
  }
  
  if (c != '\r')
  {
    if (c == '\b')
    {
      if (idx > 0)
      {
        if (put_ch != NULL)
        {
          put_ch(' ');
          put_ch(c);
        }
        idx--;
      }
    }
    else buf[idx++] = c;
  }
  else
  {
    if (put_ch != NULL)
    {
      put_ch('\n');
    }
    buf[idx] = 0;
    process_post(&serial_shell_process, serial_line_event_message, buf);
    idx = 0;
  }
  return 0;
}
#endif
