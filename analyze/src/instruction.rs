//! This module contains the types to represent baresifter output.

use std::convert::TryInto;
use std::error::Error;
use std::fmt;

/// A representation of a single instruction as it is outut by
/// Barsifter.
///
/// The internal representation is geared for small size and lack of
/// heap allocations.
#[derive(Debug, Clone, PartialEq)]
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

/// A helper function for [Instruction::increment].
#[derive(Debug, Clone, Copy)]
pub enum ByteWrapped {
    /// Incrementing the byte did not wrap.
    NoWrap,

    /// Incrementing the byte did wrap.
    Wrap,
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

    /// Return the whole backing store including trailing null bytes.
    pub fn all_bytes(&self) -> &[u8; 15] {
        &self.bytes
    }

    /// Increment an instruction byte by mutating the
    /// instruction. Return whether the byte actually wrapped to zero.
    pub fn increment_mut(&mut self, pos: usize) -> ByteWrapped {
        assert!(pos < self.len());

        let (v, overflow) = self.bytes[pos].overflowing_add(1);

        self.bytes[pos] = v;

        if overflow {
            ByteWrapped::Wrap
        } else {
            ByteWrapped::NoWrap
        }
    }

    /// The immutable version of [increment_mut]. This returns a new
    /// instruction and the wrapping indicator instead of modifying an
    /// instruction.
    pub fn increment(&self, pos: usize) -> (Instruction, ByteWrapped) {
        let mut new = self.clone();
        let wrapped = new.increment_mut(pos);

        (new, wrapped)
    }
}

/// An iterator that interpolates missing lines from Baresifter output.
#[derive(Debug)]
struct InterpolateIterator {
    /// The current instruction. This is the instruction that is
    /// returned from [next].
    current: Instruction,

    /// We iterate to this instruction (but not including it).
    target: Instruction,

    /// The byte position in the instruction byte array that we are
    /// currently incrementing. This moves from end to start through
    /// instruction.
    pos: usize,
}

impl InterpolateIterator {
    /// Construct a new instruction iterator that generates all
    /// instructions between these two.
    ///
    /// The target instruction will not be emitted.
    fn new(from: &Instruction, to: &Instruction) -> InterpolateIterator {
        assert!(from.len() > 0);

        InterpolateIterator {
            current: from.clone(),
            target: to.clone(),

            pos: from.len() - 1,
        }
    }
}

impl Iterator for InterpolateIterator {
    type Item = Instruction;

    /// Return the next instruction in the sequence.
    ///
    /// This uses the same logic as
    /// `search_engine::find_next_candidate` in the C++ baresifter
    /// code to generate the same instruction sequences.
    fn next(&mut self) -> Option<Instruction> {
        if self.current.all_bytes() == self.target.all_bytes() {
            None
        } else {
            let ret = self.current.clone();

            loop {
                if let ByteWrapped::Wrap = self.current.increment_mut(self.pos) {
                    // We wrapped and we are already at the
                    // beginning. This should not happen.
                    assert!(self.pos > 0);

                    self.pos -= 1;
                    continue;
                }

                break;
            }

            Some(ret)
        }
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

    #[test]
    fn test_instruction_iterator() -> Result<(), InvalidInstruction> {
        let instr_start = Instruction::new(0xe, &[0x0d, 0xff, 0xfe])?;
        let instr_end = Instruction::new(0xe, &[0x0f, 0x00, 0x00])?;

        let inbetween: Vec<Instruction> =
            InterpolateIterator::new(&instr_start, &instr_end).collect();

        assert_eq!(
            inbetween,
            vec![
                Instruction::new(0xe, &[0x0d, 0xff, 0xfe])?,
                Instruction::new(0xe, &[0x0d, 0xff, 0xff])?,
                Instruction::new(0xe, &[0x0e, 0x00, 0x00])?,
            ]
        );

        Ok(())
    }
}
