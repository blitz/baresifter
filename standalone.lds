OUTPUT_FORMAT("elf64-x86-64")
OUTPUT_ARCH("i386:x86-64")
ENTRY(_start)
EXTERN(_start)

PHDRS {
      rwx PT_LOAD ;
}

SECTIONS
{
      ENTRY(_start)

      /* 2 MB */
      . = 0x200000;
      _image_start = .;

      . += SIZEOF_HEADERS;

      .text :
      {
        *(.text._start)
        *(.text .text.*);
        *(.rodata .rodata.*);
      } : rwx

      .init_array :
      {
        _init_array_start = .;
        KEEP(SORT_BY_INIT_PRIORITY(*)(.init_array.*))
        KEEP(*(.init_array))
        _init_array_end = .;
      } : rwx

      .data :
      {
        *(.data .data.*);
      }

      .bss :
      {
        *(.bss  .bss.*);
        . = ALIGN(0x200000);
        _image_end = .;
      }

      /DISCARD/ :
      {
        *(.not.GNU-stack)
        *(.comment)
      }
}
