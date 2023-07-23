
use std::{
    os::windows::process::*,
    process::*,
    io::*,
};

fn main() {
    
    let pid = std::process::id().to_string();
    let output = Command::new("C:\\Users\\ols3\\prog\\DuplicateFileHandleAsAdmin\\x64\\Debug\\DuplicateFileHandleAsAdmin.exe")
        .args([&pid, ".\\moin.txt"])
        .stdout(Stdio::piped())
        .stderr(Stdio::inherit())
        .output()
        .expect("failed to execute process");

    println!("status: {}", output.status);
    std::io::stdout().write_all(&output.stdout).unwrap();
}
