//! This module implements a parser for Baresifter output.

use nom::{
    bytes::complete::{tag, take_while_m_n},
    combinator::map_res,
    multi::separated_list1,
    Finish, IResult,
};
use std::str::FromStr;

use crate::types::{Instruction, InvalidInstruction};

fn from_hex(input: &str) -> Result<u8, std::num::ParseIntError> {
    u8::from_str_radix(input, 16)
}

fn is_hex_digit(c: char) -> bool {
    c.is_digit(16)
}

fn hex_byte(input: &str) -> IResult<&str, u8> {
    map_res(take_while_m_n(2, 2, is_hex_digit), from_hex)(input)
}

fn instruction(input: &str) -> IResult<&str, Instruction> {
    let (input, _) = tag("EXC ")(input)?;
    let (input, exc) = hex_byte(input)?;
    let (input, _) = tag(" OK | ")(input)?;
    let (input, bytes): (&str, Vec<u8>) = separated_list1(tag(" "), hex_byte)(input)?;

    Ok((
        input,
        // TODO I wish I knew how to report failure to create an
        // instruction in terms of the nom error types.
        Instruction::new(exc, &bytes).expect("failed to create instruction"),
    ))
}

impl FromStr for Instruction {
    type Err = InvalidInstruction;

    fn from_str(s: &str) -> std::result::Result<Self, InvalidInstruction> {
        Ok(instruction(s)
            .finish()
            .map_err(|_| InvalidInstruction::default())?
            .1)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use anyhow::Result;

    #[test]
    fn test_parse() -> Result<()> {
        let i = instruction("EXC 0D OK | 00 04 2E")?.1;

        assert_eq!(i.exception(), 0xd);
        assert_eq!(i.bytes(), &[0x00, 0x04, 0x2e]);

        Ok(())
    }
}
