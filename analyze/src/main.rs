use anyhow::Result;
use iced_x86;
use std::io::{self, BufRead};
use std::str::FromStr;

mod instruction;
mod parser;

use instruction::Instruction;

fn iced_decode(instr: &Instruction) -> iced_x86::Instruction {
    iced_x86::Decoder::with_ip(64, instr.all_bytes(), 0, 0).decode()
}

fn main() -> Result<()> {
    eprintln!("Waiting for input on stdin...");

    let stdin = io::stdin();

    let instrs = stdin
        .lock()
        .lines()
        .filter_map(|l| -> Option<Instruction> { Instruction::from_str(&l.ok()?).ok() })
        .map(|i| {
            let iced_instr = iced_decode(&i);
            (i, iced_instr)
        })
        .filter(|(i, iced_instr)| i.exception() != 0x06 && i.len() != iced_instr.len());

    for (bare_instr, iced_instr) in instrs {
        println!(
            "{:02x?} {:02} | {:02} {}",
            &bare_instr.all_bytes(),
            bare_instr.len(),
            iced_instr.len(),
            iced_instr
        );
    }

    Ok(())
}
