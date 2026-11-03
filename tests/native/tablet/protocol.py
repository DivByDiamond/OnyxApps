"""Exercise the same persistent native process used by the graphical harness."""
from pathlib import Path
import subprocess
import tempfile

with tempfile.TemporaryDirectory(prefix='onyx-tablet-test-') as directory:
    path = Path(directory) / 'frame.ppm'
    process = subprocess.Popen(['./demo', str(path), '--interactive'],
                               stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                               text=True)
    try:
        assert process.stdout.readline().strip() == 'READY'
        initial = path.read_bytes()
        assert initial.startswith(b'P6\n240 112\n255\n')
        assert len(initial) == len(b'P6\n240 112\n255\n') + 240 * 112 * 3

        def frame(command):
            process.stdin.write(command + '\n')
            process.stdin.flush()
            assert process.stdout.readline().strip() == 'READY'
            return path.read_bytes()

        stopped = frame('10 70')
        assert stopped != initial
        offline = frame('offline')
        assert offline != stopped
        assert frame('10 70') == offline
        assert frame('refresh') == stopped
    finally:
        process.stdin.close()
        assert process.wait(timeout=3) == 0
print('tablet: persistent pointer/offline/refresh protocol passed')
