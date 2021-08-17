/// Convert a slice of bytes to a space separated hexadecimal string.
pub fn slice_as_hex_string(slice: &[u8]) -> String {
    let mut out = String::new();

    // Each byte gets 2 hex digits and a space, minus one space for
    // the end. This allocates some bytes to many with zero or one
    // element slices, but that's fine.
    out.reserve(slice.len() * 3);

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

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn slice_as_hex_string_works() {
        assert_eq!(slice_as_hex_string(&[]), "");
        assert_eq!(slice_as_hex_string(&[0x12]), "12");
        assert_eq!(slice_as_hex_string(&[0x12, 0x34]), "12 34");
    }
}
