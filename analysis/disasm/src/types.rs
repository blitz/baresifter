//! This module contains the types to represent baresifter output.

use std::convert::TryInto;
use std::error::Error;
use std::fmt;

/// A representation of a single instruction as it is outut by
/// Barsifter.
///
/// The internal representation is geared for small size and lack of
/// heap allocations.
#[derive(Debug, PartialEq)]
pub struct Instruction {
    /// This is actually two fields: the exception (top 4 bits) and
    /// the length of the instruction bytes (lower 4 bits).
    exc_len: u8,

    /// The instruction bytes.
    bytes: [u8; 15],
}

#[derive(Debug, Default, PartialEq, Eq)]
pub struct InvalidInstruction {}

impl Error for InvalidInstruction {}

impl fmt::Display for InvalidInstruction {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "Invalid instruction")
    }
}

/// Convert a slice into a instruction byte array.
///
/// This function assumes that the caller already checked the length
/// of the slice.
fn slice_to_arr(s: &[u8]) -> [u8; 15] {
    let mut out: [u8; 15] = [0; 15];

    out[..s.len()].copy_from_slice(s);

    out
}

impl Instruction {
    pub fn new(exception: u8, bytes: &[u8]) -> Result<Self, InvalidInstruction> {
        let len = bytes.len();

        if len >= 16 || exception >= 16 {
            Err(InvalidInstruction::default())
        } else {
            // The unwraps are safe, because we already checked the ranges above.
            Ok(Instruction {
                exc_len: exception << 4 | TryInto::<u8>::try_into(len).unwrap(),
                bytes: slice_to_arr(bytes),
            })
        }
    }

    /// Return the exception that was thrown when this instruction was executed.
    pub fn exception(&self) -> u8 {
        self.exc_len >> 4
    }

    /// Return the length of the instruction in bytes.
    pub fn len(&self) -> usize {
        (self.exc_len & 0xF).into()
    }

    /// Return the instruction as a byte slice.
    pub fn bytes(&self) -> &[u8] {
        &self.bytes[..self.len()]
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_ok_instruction() -> Result<(), InvalidInstruction> {
        let bytes = [0x0e, 0x01, 0xf9];
        let instr = Instruction::new(0xe, &bytes)?;

        assert_eq!(instr.exception(), 0x0e);
        assert_eq!(instr.len(), 3);
        assert_eq!(instr.bytes(), &bytes);

        Ok(())
    }

    #[test]
    fn test_invalid_instruction() {
        // Invalid exception.
        assert_eq!(
            Instruction::new(0xff, &[0x90]),
            Err(InvalidInstruction::default())
        );

        // Instruction too long.
        assert_eq!(
            Instruction::new(0x0e, &[0; 16]),
            Err(InvalidInstruction::default())
        );
    }
}
