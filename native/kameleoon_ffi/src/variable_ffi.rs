use kameleoon_core::types::{variable::JsonValue, Variable};

use crate::raw_str_ffi::BorrowedStr;

/// View of a feature variable. Strings point into the `Arc<str>` data of the
/// core value retained by the enclosing payload's `owner`.
#[repr(C)]
#[derive(Clone, Copy)]
pub struct FfiVariable {
    key: BorrowedStr,
    kind: BorrowedStr,
    value: FfiJsonValue,
}

impl From<&Variable> for FfiVariable {
    fn from(var: &Variable) -> Self {
        FfiVariable {
            key: var.key.as_ref().into(),
            kind: var.kind.as_ref().into(),
            value: (&var.value).into(),
        }
    }
}

#[repr(C, u32)]
#[derive(Clone, Copy)]
pub enum FfiJsonValue {
    Boolean(bool),
    Number(f64),
    String(BorrowedStr),
    JSON(BorrowedStr),
    JS(BorrowedStr),
    CSS(BorrowedStr),
}

impl From<&JsonValue> for FfiJsonValue {
    fn from(jvalue: &JsonValue) -> Self {
        match jvalue {
            JsonValue::Boolean(value) => FfiJsonValue::Boolean(*value),
            JsonValue::Number(value) => FfiJsonValue::Number(*value),
            JsonValue::String(value) => FfiJsonValue::String(value.as_ref().into()),
            JsonValue::Json(value) => FfiJsonValue::JSON(value.as_ref().into()),
            JsonValue::Js(value) => FfiJsonValue::JS(value.as_ref().into()),
            JsonValue::Css(value) => FfiJsonValue::CSS(value.as_ref().into()),
        }
    }
}

#[cfg(test)]
mod tests {
    use std::sync::Arc;

    use super::*;

    #[test]
    fn variable_views_preserve_value_kinds_and_borrow_string_bytes() {
        let text: Arc<str> = Arc::from("value\0with Unicode: \u{1f980}");
        let values = [
            JsonValue::Boolean(true),
            JsonValue::Number(42.5),
            JsonValue::String(text.clone()),
            JsonValue::Json(text.clone()),
            JsonValue::Js(text.clone()),
            JsonValue::Css(text.clone()),
            JsonValue::String(Arc::from("")),
        ];

        for value in values {
            let var = Variable {
                key: Arc::from("variable"),
                kind: Arc::from("kind"),
                value,
            };
            let view = FfiVariable::from(&var);
            assert_eq!(view.key.str as *const u8, var.key.as_ptr());
            assert_eq!(view.kind.str as *const u8, var.kind.as_ptr());

            match (&var.value, view.value) {
                (JsonValue::Boolean(expected), FfiJsonValue::Boolean(actual)) => assert_eq!(*expected, actual),
                (JsonValue::Number(expected), FfiJsonValue::Number(actual)) => assert_eq!(*expected, actual),
                (JsonValue::String(expected), FfiJsonValue::String(actual))
                | (JsonValue::Json(expected), FfiJsonValue::JSON(actual))
                | (JsonValue::Js(expected), FfiJsonValue::JS(actual))
                | (JsonValue::Css(expected), FfiJsonValue::CSS(actual)) => {
                    assert_eq!(<&str>::from(actual), expected.as_ref());
                    if !expected.is_empty() {
                        assert_eq!(actual.str as *const u8, expected.as_ptr());
                    }
                    assert_eq!(actual.size as usize, expected.len());
                }
                _ => panic!("the view changed the variable value kind"),
            }
        }
    }
}
