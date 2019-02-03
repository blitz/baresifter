OUTPUT_FORMAT("elf64-x86-64")
OUTPUT_ARCH("i386:x86-64")
ENTRY(_start)
EXTERN(_start)

PHDRS {
      rx PT_LOAD FILEHDR PHDRS;
      rw PT_LOAD;
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

        . = ALIGN(8);
        _init_array_start = .;
        KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*)))
        KEEP(*(.init_array))
        _init_array_end = .;
      } : rx

      .data :
      {
        *(.note.gnu.build-id);
        *(.data .data.*);
      } : rw

      .bss :
      {
        *(.bss  .bss.*);
      } : rw

      _image_end = .;

      /DISCARD/ :
      {
        *(.comment)
        *(.eh_frame)
      }
}
