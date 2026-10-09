use std::ffi::c_char;

use crate::FreeRaw;

// MARK: - StrPtr

pub type StrPtr = *const c_char;

/// # Safety
/// A non-null `ptr` must point to `len` bytes of valid UTF-8 that outlive the
/// returned borrow; the foreign side guarantees both. A null `ptr` reads as
/// the empty string, so a `NULL` passed where a string is required cannot
/// become an invalid slice.
unsafe fn raw_to_str<'a>(ptr: StrPtr, len: u32) -> &'a str {
    if ptr.is_null() {
        return "";
    }
    std::str::from_utf8_unchecked(std::slice::from_raw_parts(ptr as *const u8, len as usize))
}

fn raw_to_option_str<'a>(ptr: StrPtr, len: u32) -> Option<&'a str> {
    if ptr.is_null() {
        None
    } else {
        Some(unsafe { raw_to_str(ptr, len) })
    }
}

// MARK: - BorrowedStr (never freed by the receiver)
// Inputs and callback arguments borrow for the duration of the call. Strings
// inside structured results (`FfiVariation`, `FfiDataFile`, …) point into the
// core value retained by the result's `owner` and stay valid until that
// result's `*__free` call.

#[repr(C)]
#[derive(Clone, Copy)]
pub struct BorrowedStr {
    pub str: StrPtr,
    pub size: u32,
}

impl BorrowedStr {
    pub const NULL: BorrowedStr = BorrowedStr {
        str: std::ptr::null(),
        size: 0,
    };
}

impl From<&str> for BorrowedStr {
    fn from(value: &str) -> Self {
        if value.is_empty() {
            return BorrowedStr {
                str: std::ptr::NonNull::<u8>::dangling().as_ptr() as StrPtr,
                size: 0,
            };
        }

        BorrowedStr {
            str: value.as_ptr() as StrPtr,
            size: value.len() as u32,
        }
    }
}

impl From<Option<&str>> for BorrowedStr {
    fn from(value: Option<&str>) -> Self {
        match value {
            Some(value) => value.into(),
            None => BorrowedStr::NULL,
        }
    }
}

impl From<BorrowedStr> for &str {
    fn from(value: BorrowedStr) -> Self {
        unsafe { raw_to_str(value.str, value.size) }
    }
}

impl From<BorrowedStr> for String {
    fn from(value: BorrowedStr) -> Self {
        <&str>::from(value).to_owned()
    }
}

impl From<BorrowedStr> for Option<&str> {
    fn from(value: BorrowedStr) -> Self {
        raw_to_option_str(value.str, value.size)
    }
}

impl From<BorrowedStr> for Option<String> {
    fn from(value: BorrowedStr) -> Self {
        <Option<&str>>::from(value).map(|s| s.to_owned())
    }
}

// MARK: - OwnedStr (a `Box<str>` handed to the foreign side; released by
// `string__free`, which gives the box back to Rust)

#[repr(C)]
#[derive(Clone, Copy)]
pub struct OwnedStr {
    pub str: StrPtr,
    pub size: u32,
}

impl OwnedStr {
    pub const NULL: OwnedStr = OwnedStr {
        str: std::ptr::null(),
        size: 0,
    };
}

impl FreeRaw for OwnedStr {
    fn free(&self) {
        if self.str.is_null() {
            return;
        }
        // SAFETY: a non-null `OwnedStr` only comes from `From<String>`, i.e.
        // from `Box::<str>::into_raw` with this pointer and length.
        drop(unsafe { Box::from_raw(std::ptr::slice_from_raw_parts_mut(self.str as *mut u8, self.size as usize)) });
    }
}

/// Releases an `OwnedStr` handed out through an `out` parameter.
#[no_mangle]
pub extern "C" fn string__free(value: OwnedStr) {
    value.free();
}

impl From<OwnedStr> for &str {
    fn from(value: OwnedStr) -> Self {
        unsafe { raw_to_str(value.str, value.size) }
    }
}

impl From<String> for OwnedStr {
    /// Hands the string's own buffer to the caller instead of copying it
    /// (`into_boxed_str` only trims spare capacity). An empty string yields a
    /// dangling, non-null pointer and no allocation.
    fn from(s: String) -> Self {
        let boxed = s.into_boxed_str();
        let size = boxed.len() as u32;
        OwnedStr {
            str: Box::into_raw(boxed) as *mut u8 as StrPtr,
            size,
        }
    }
}

// MARK: - ForeignOwnedStr (comes from FFI and free method should be called)

#[repr(C)]
#[derive(Clone, Copy)]
pub struct ForeignOwnedStr {
    pub str: StrPtr,
    pub size: u32,
}

impl From<ForeignOwnedStr> for &str {
    fn from(value: ForeignOwnedStr) -> Self {
        unsafe { raw_to_str(value.str, value.size) }
    }
}

#[cfg(test)]
mod tests {
    use super::{BorrowedStr, OwnedStr};
    use crate::{test_utils::watch, FreeRaw};

    #[test]
    fn owned_str_from_string_round_trips_and_frees() {
        let mut value = String::with_capacity(32);
        value.push_str("visitor code");

        let raw = OwnedStr::from(value);
        let buffer = watch(raw.str);

        assert_eq!(<&str>::from(raw), "visitor code");
        assert_eq!(raw.size, 12);

        raw.free();

        assert_eq!(buffer.freed_size(), Some(12), "free must release the shrunk `Box<str>` buffer");
    }

    #[test]
    fn owned_str_from_empty_string_has_no_allocation() {
        let raw = OwnedStr::from(String::new());
        let buffer = watch(raw.str);

        assert!(!raw.str.is_null());
        assert_eq!(raw.size, 0);

        raw.free();

        assert_eq!(buffer.freed_size(), None, "an empty `Box<str>` is dangling and never deallocated");
    }

    #[test]
    fn borrowed_str_points_to_source_bytes() {
        let value = "network failed";
        let raw = BorrowedStr::from(value);

        assert_eq!(raw.str as *const u8, value.as_ptr());
        assert_eq!(raw.size, value.len() as u32);
    }

    #[test]
    fn borrowed_str_preserves_empty_string_shape() {
        let raw = BorrowedStr::from("");

        assert!(!raw.str.is_null());
        assert_eq!(raw.size, 0);
    }

    #[test]
    fn borrowed_str_converts_to_owned_string() {
        let value = "network failed";
        let raw = BorrowedStr::from(value);

        assert_eq!(String::from(raw), value);
    }

    #[test]
    fn borrowed_str_from_none_is_null() {
        let raw = BorrowedStr::from(None);

        assert!(raw.str.is_null());
        assert_eq!(raw.size, 0);
    }

    #[test]
    fn null_borrowed_str_reads_as_empty_where_a_string_is_required() {
        assert_eq!(<&str>::from(BorrowedStr::NULL), "");
        assert_eq!(String::from(BorrowedStr::NULL), "");
        assert_eq!(<Option<&str>>::from(BorrowedStr::NULL), None);
    }
}
