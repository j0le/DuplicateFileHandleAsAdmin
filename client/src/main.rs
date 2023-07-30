
use core::num::*;

use std::{
    os::windows::process::*,
    process::*,
    io::*,
    ffi::OsStr,
};

use windows::Win32::UI::Shell::*;
use windows::core::PCWSTR;

use interprocess::os::windows::named_pipe as named_pipe;

fn main() -> std::io::Result<()>{

    let debug = false;
    if debug {
        while ! unsafe{windows::Win32::System::Diagnostics::Debug::IsDebuggerPresent()}.as_bool()
        {
            use std::{thread, time};
            let sleep_duration = time::Duration::from_millis(100);

            println!("waiting for debugger!");
            thread::sleep(sleep_duration);
        }
        unsafe{windows::Win32::System::Diagnostics::Debug::DebugBreak();}
    }
    
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

    let pipe_name : String = format!("dfhaa_{}", pid);

    let pipe_listener = named_pipe::PipeListenerOptions::new()
        .name(OsStr::new(&pipe_name))
        .mode(named_pipe::PipeMode::Bytes)
        .instance_limit(NonZeroU8::new(2))
        .accept_remote(false)
        .wait_timeout(unsafe{NonZeroU32::new_unchecked(20000)})
        .create::<named_pipe::ByteReaderPipeStream>()?;

    {
        use std::iter::once;
        use std::os::windows::ffi::OsStrExt;
        use windows::Win32::Foundation::HWND;
        use windows::Win32::UI::WindowsAndMessaging::SW_SHOW;
        let runas: [u16; 6] = [0x72u16, 0x75u16, 0x6eu16, 0x61u16, 0x73u16, 0u16];
        let prog : Vec<u16> = OsStr::new(prog_dup_file_handle_as_admin).encode_wide().chain(once(0u16)).collect();

        let parameters : String = format!("{}  \"{}\"", pid, "C:\\Users\\ols3\\Desktop\\moin.txt");
        let os_parameters : Vec<u16> = OsStr::new(&parameters).encode_wide().chain(once(0u16)).collect();

        let as_admin : bool = false;
        let operation = if as_admin {
            PCWSTR::from_raw(runas.as_ptr())
        } else {
            PCWSTR::null()
        };

        let hwnd = HWND(0);
        // TODO: handle the result of SehllExecuteW
        let shell_exec_result = unsafe {
            ShellExecuteW(
                hwnd,
                operation,
                PCWSTR::from_raw(prog.as_ptr()),
                PCWSTR::from_raw(os_parameters.as_ptr()),
                PCWSTR::null(),
                SW_SHOW,
            )
        };
    }

    let mut pipe = match pipe_listener.accept() {
        Ok(x) => x,
        Err(_) => {println!("it does fail with accept"); panic!();},
    };
    let mut buf : [u8; 8]= [0u8; 8];
    pipe.read_exact(&mut buf)?;

    let handle_as_unsigned_integer = u64::from_ne_bytes(buf);

    {
        use std::os::windows::io::FromRawHandle;
        use std::fs::File;
        use std::os::windows::io::RawHandle;

        let h_file : RawHandle = handle_as_unsigned_integer as RawHandle;

        let mut f = unsafe{File::from_raw_handle(h_file)};

        write!(&mut f, "Let’s write to the file, hey!")?;
        f.sync_data()?;
    }

    Result::Ok(())
}
