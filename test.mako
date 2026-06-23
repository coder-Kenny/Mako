// ==== Mako Lexer Testfile ====
// NOTE: This source file serves as a syntactic baseline for testing.
// The concrete language syntax and grammar rules are subject to modification
// during subsequent stages of the compiler architecture development.
import std.io;

export struct FileDescriptor {
    fd: IntegerLiteral
}

impl FileDescriptor {
    fn close(self) -> Error {
        let status = 0;
        return status;
    }
}

fn process_data(var buffer: [IntegerLiteral]) {
    let x = 42;
    let y = 3.1415;
    let is_valid = true;

    let mask = 0b1010 & 0x0F;
    let shift = 1 << 4;

    if (x == 42 && is_valid || y != 0.0) {
        x = x + 1;
    } else {
        x = x % 2;
    }

    match buffer {
        case Empty => return;
        case Data(payload) => {
            let processed = move payload;
        }
    }
}

let a = 5;
let b = a->field;
let c = a - > 2;
let d = 10 == 20;
let e = 10 = = 20;
let f = 5 <= 10;
let g = 5 < = 10;
let h = a => b;

let bad_char = @;
let bad_number = 0b1020;
let bad_hex = 0xG1;
let bad_octal = 0o89;
let unclosed_str = "hello;