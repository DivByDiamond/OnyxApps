#!/usr/bin/env python3
"""Native graphical mouse harness. Run after `make ... tablet/demo`."""
import pathlib
import subprocess
import sys
import tempfile
import tkinter as tk

binary = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else
                      'tests/native/tablet/demo').resolve()
with tempfile.TemporaryDirectory(prefix='onyx-tablet-') as directory:
    path = pathlib.Path(directory) / 'dashboard.ppm'
    process = subprocess.Popen([str(binary), str(path), '--interactive'],
                               stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                               text=True)
    root = tk.Tk()
    root.title('Onyx tablet — LOCAL MOCK')
    canvas = tk.Label(root)
    canvas.pack()

    def redraw(command=None):
        if command is not None:
            process.stdin.write(command + '\n')
            process.stdin.flush()
        if process.stdout.readline().strip() != 'READY':
            root.destroy()
            return
        picture = tk.PhotoImage(file=str(path)).zoom(3)
        canvas.configure(image=picture)
        canvas.image = picture

    canvas.bind('<Button-1>', lambda e: redraw(f'{e.x // 3} {e.y // 3}'))
    tk.Button(root, text='Simulate timeout', command=lambda: redraw('offline')).pack()
    tk.Button(root, text='Refresh local mock', command=lambda: redraw('refresh')).pack()
    redraw()
    try:
        root.mainloop()
    finally:
        process.stdin.close()
        process.wait(timeout=3)
