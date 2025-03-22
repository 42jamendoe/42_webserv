#!/usr/bin/env python

import os
import sys

upload_dir = os.getenv("UPLOAD_DIR")
if not upload_dir:
    sys.stdout.buffer.write(b"Status: 500 Internal Server Error\nContent-Type: text/plain\n\nUPLOAD_DIR environment variable is not set")
    sys.exit(1)

if not os.path.exists(upload_dir):
    try:
        os.makedirs(upload_dir)
    except Exception as e:
        sys.stdout.buffer.write(f"Status: 500 Internal Server Error\nContent-Type: text/plain\n\nFailed to create upload directory: {str(e)}".encode())
        sys.exit(1)

transfer_encoding = os.getenv("HTTP_TRANSFER_ENCODING", "").lower()
is_chunked = transfer_encoding == "chunked"

def read_chunked_input():
    data = b""
    while True:
        chunk_size_line = sys.stdin.buffer.readline().strip()
        if not chunk_size_line:
            break
        chunk_size = int(chunk_size_line, 16)
        if chunk_size == 0:
            break
        chunk_data = sys.stdin.buffer.read(chunk_size)
        data += chunk_data
        sys.stdin.buffer.readline()
    return data

if is_chunked:
    try:
        body = read_chunked_input()
    except Exception as e:
        sys.stdout.buffer.write(f"Status: 400 Bad Request\nContent-Type: text/plain\n\nFailed to read chunked data: {str(e)}".encode())
        sys.exit(1)
else:
    try:
        content_length = int(os.getenv("CONTENT_LENGTH", "0"))
    except ValueError:
        sys.stdout.buffer.write(b"Status: 400 Bad Request\nContent-Type: text/plain\n\nInvalid CONTENT_LENGTH")
        sys.exit(1)

    if content_length <= 0:
        sys.stdout.buffer.write(b"Status: 400 Bad Request\nContent-Type: text/plain\n\nNo data received")
        sys.exit(1)

    body = sys.stdin.buffer.read(content_length)

file_path = os.path.join(upload_dir, "uploaded_file.bin")

try:
    with open(file_path, "wb") as f:
        f.write(body)

    response = f"Status: 201 Created\nContent-Type: text/plain\n\nFile uploaded successfully: {file_path}"
    sys.stdout.buffer.write(response.encode())

except Exception as e:
    error_msg = f"Status: 500 Internal Server Error\nContent-Type: text/plain\n\nFailed to save file: {str(e)}"
    sys.stdout.buffer.write(error_msg.encode())