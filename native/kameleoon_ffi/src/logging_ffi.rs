use kameleoon_core::logging::{DefaultLogger, KameleoonLogger, LogLevel, Logger};

use crate::callback_ffi::FfiLogCallback;

struct ExternalLogger {
    // The message borrows from the logging call and is valid only for the
    // duration of the callback invocation; the receiver must not free it.
    logfunc: FfiLogCallback,
}

impl Logger for ExternalLogger {
    fn log(&self, log_level: LogLevel, message: &str) {
        (self.logfunc.func)(self.logfunc.context, log_level as u8, message.into());
    }
}

#[no_mangle]
pub unsafe extern "C" fn logger__set_log_level(log_level: u8) {
    KameleoonLogger::set_log_level(LogLevel::from(log_level));
}

#[no_mangle]
pub unsafe extern "C" fn logger__set_logfunc(logfunc: FfiLogCallback) {
    KameleoonLogger::set_logger(Box::new(ExternalLogger { logfunc }));
}

/// Restores the default (stdout) logger. After this call the previously
/// registered external log function is never invoked again.
#[no_mangle]
pub unsafe extern "C" fn logger__reset_logfunc() {
    KameleoonLogger::set_logger(Box::new(DefaultLogger {}));
}
