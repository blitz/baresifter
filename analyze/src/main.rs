use anyhow::{anyhow, Context, Result};
use clap::{crate_version, Clap};
use iced_x86;
use std::{
    fs::File,
    io::{BufRead, BufReader},
    path::PathBuf,
    str::FromStr,
};

mod instruction;
mod parser;

use instruction::Instruction;

#[derive(Clap, Debug)]
#[clap(version = crate_version!(), author = "Julian Stecklina <js@alien8.de>")]
struct Opts {
    /// The address width to use.
    #[clap(long, default_value = "64")]
    bits: u8,

    /// The input file to parse.
    input_file: PathBuf,
}

fn get_decoder(bits: u8) -> Result<Box<dyn Fn(&Instruction) -> iced_x86::Instruction>> {
    match bits {
        32 | 64 => Ok(Box::new(move |instr: &Instruction| {
            iced_x86::Decoder::with_ip(bits.into(), instr.all_bytes(), 0, 0).decode()
        })),
        _ => Err(anyhow!(
            "Got {} as bits parameter, but we only support 32 and 64.",
            bits
        )),
    }
}

/// Convert a slice of bytes to a space separated hexadecimal string.
fn slice_as_hex_string(slice: &[u8; 15]) -> String {
    let mut out = String::new();

    // Each byte gets 2 hex digits and a space, minus one space for
    // the end.
    out.reserve(slice.len() * 3 - 1);

    slice
        .iter()
        .map(|v| format!("{:02x}", v))
        .fold(String::new(), |mut acc, hex| {
            if !acc.is_empty() {
                acc.push(' ');
            }

            acc.push_str(&hex);
            acc
        })
}

fn main() -> Result<()> {
    let opts: Opts = Opts::parse();

    let decoder = get_decoder(opts.bits)?;
    let file = File::open(&opts.input_file)
        .with_context(|| format!("Failed to open input file: {}", opts.input_file.display()))?;
    let file = BufReader::new(file);

    let instrs = file
        .lines()
        .filter_map(|l| -> Option<Instruction> { Instruction::from_str(&l.ok()?).ok() })
        .map(|i| {
            let iced_instr = decoder(&i);
            (i, iced_instr)
        })
        .filter(|(i, iced_instr)| i.exception() != 0x06 && i.len() != iced_instr.len());

    for (bare_instr, iced_instr) in instrs {
        println!(
            "{} | {:02} ≠ {:02} {}",
            slice_as_hex_string(&bare_instr.all_bytes()),
            bare_instr.len(),
            iced_instr.len(),
            iced_instr
        );
    }

    Ok(())
}
