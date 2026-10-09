//! Arrays cross the boundary as `Box<[T]>`: pointer plus length, handed out
//! with `Box::into_raw` and reclaimed with `Box::from_raw`, so the C side sees
//! no allocator details (no capacity). Maps are arrays of `FfiKeyValuePair`.

#[repr(C)]
#[derive(Clone, Copy)]
pub struct FfiArray<T> {
    pub ptr: *const T,
    pub len: u32,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct FfiKeyValuePair<K: Sized, V: Sized> {
    pub key: K,
    pub value: V,
}

impl<T> FfiArray<T> {
    /// Borrows `slice` for the duration of a call (callback payloads).
    pub fn borrowed(slice: &[T]) -> Self {
        Self {
            ptr: if slice.is_empty() { std::ptr::null() } else { slice.as_ptr() },
            len: slice.len() as u32,
        }
    }

    /// Borrows the elements; valid while the original memory is. A null
    /// pointer reads as empty whatever `len` says, so a caller that sends
    /// `{NULL, n}` cannot produce an invalid slice.
    pub fn as_slice(&self) -> &[T] {
        if self.len == 0 || self.ptr.is_null() {
            return &[];
        }

        unsafe { std::slice::from_raw_parts(self.ptr, self.len as usize) }
    }

    /// Releases the array buffer only. Elements are plain views (or `Copy`
    /// values) and own nothing themselves; whatever they borrow from is kept
    /// alive by the payload's `owner`.
    pub fn shallow_free(self) {
        if self.ptr.is_null() {
            return;
        }
        // SAFETY: a non-null array only comes from `From<Vec<T>>`, i.e. from
        // `Box::<[T]>::into_raw` with this pointer and length.
        drop(unsafe { Box::from_raw(std::ptr::slice_from_raw_parts_mut(self.ptr as *mut T, self.len as usize)) });
    }
}

impl<T> From<Vec<T>> for FfiArray<T> {
    /// `into_boxed_slice` only trims spare capacity; vectors collected from
    /// exact-size iterators (every producer in this crate) are not moved.
    fn from(v: Vec<T>) -> Self {
        let boxed = v.into_boxed_slice();
        let len = boxed.len() as u32;
        Self {
            ptr: Box::into_raw(boxed) as *const T,
            len,
        }
    }
}

#[cfg(test)]
mod tests {
    use std::mem::size_of;

    use super::FfiArray;
    use crate::test_utils::watch;

    #[test]
    fn as_slice_accepts_null_empty_array() {
        let array = FfiArray::<u8> {
            ptr: std::ptr::null(),
            len: 0,
        };

        assert!(array.as_slice().is_empty());
    }

    #[test]
    fn as_slice_reads_null_pointer_as_empty_regardless_of_len() {
        let array = FfiArray::<u8> {
            ptr: std::ptr::null(),
            len: 3,
        };

        assert!(array.as_slice().is_empty());
    }

    #[test]
    fn as_slice_reads_non_empty_array() {
        let values = [1_u8, 2, 3];
        let array = FfiArray {
            ptr: values.as_ptr(),
            len: values.len() as u32,
        };

        assert_eq!(array.as_slice(), values);
    }

    #[test]
    fn borrowed_uses_slice_memory_without_taking_ownership() {
        let values = [1_u8, 2, 3];
        let array = FfiArray::borrowed(&values);

        assert_eq!(array.ptr, values.as_ptr());
        assert_eq!(array.len, values.len() as u32);
        assert_eq!(array.as_slice(), values);
    }

    #[test]
    fn borrowed_empty_array_uses_null_pointer() {
        let values: [u8; 0] = [];
        let array = FfiArray::borrowed(&values);

        assert!(array.ptr.is_null());
        assert_eq!(array.len, 0);
        assert!(array.as_slice().is_empty());
    }

    #[test]
    fn owned_array_round_trips_through_box_and_frees() {
        let values: Vec<u32> = (0..4).collect();
        let ptr = values.as_ptr();
        let buffer = watch(ptr);

        let array = FfiArray::from(values);

        assert_eq!(array.ptr, ptr, "an exact-capacity vector keeps its buffer");
        assert_eq!(array.as_slice(), [0, 1, 2, 3]);
        assert_eq!(buffer.freed_size(), None, "the buffer must survive the handover");

        array.shallow_free();

        assert_eq!(buffer.freed_size(), Some(4 * size_of::<u32>()), "shallow_free must return the whole buffer");
    }

    #[test]
    fn empty_owned_array_has_no_allocation() {
        let array = FfiArray::from(Vec::<u32>::new());
        let buffer = watch(array.ptr);

        assert!(!array.ptr.is_null());
        assert_eq!(array.len, 0);

        array.shallow_free();

        assert_eq!(buffer.freed_size(), None, "a dangling empty slice is never passed to the allocator");
    }
}
