
use std::{
    os::windows::process::*,
    process::*,
    io::*,
};

use windows::Win32::UI::Shell::*;
use windows::core::PCWSTR;

fn main() {
    
    let pid = std::process::id().to_string();
    let prog_dup_file_handle_as_admin = "C:\\Users\\ols3\\prog\\DuplicateFileHandleAsAdmin\\x64\\Debug\\DuplicateFileHandleAsAdmin.exe";
    //let output = Command::new(prog_dup_file_handle_as_admin)
    //    .args([&pid, ".\\moin.txt"])
    //    .stdout(Stdio::piped())
    //    .stderr(Stdio::inherit())
    //    .output()
    //    .expect("failed to execute process");
    //
    //println!("status: {}", output.status);
    //std::io::stdout().write_all(&output.stdout).unwrap();

    {
        use std::ffi::OsStr;
        use std::iter::once;
        use std::os::windows::ffi::OsStrExt;
        use windows::Win32::Foundation::HWND;
        use windows::Win32::UI::WindowsAndMessaging::SW_SHOW;
        let runas: [u16; 6] = [0x72u16, 0x75u16, 0x6eu16, 0x61u16, 0x73u16, 0u16];
        let prog : Vec<u16> = OsStr::new(prog_dup_file_handle_as_admin).encode_wide().chain(once(0u16)).collect();
        let hwnd = HWND(0);
        let shell_exec_result = unsafe {
            ShellExecuteW(
                hwnd,
                PCWSTR::from_raw(runas.as_ptr()),
                PCWSTR::from_raw(prog.as_ptr()),
                PCWSTR::null(),
                PCWSTR::null(),
                SW_SHOW,
            )
        };
    }
}
