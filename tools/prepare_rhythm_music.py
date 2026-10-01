"""Build local audio excerpts and onset-based draft charts from user-supplied MP3s.

Section boundaries are editable draft timings, not semantic vocal recognition.
Run with Python + numpy/scipy and LAME. No recording is uploaded.
"""
import concurrent.futures, json, pathlib, subprocess, tempfile, wave
import numpy as np
from scipy.signal import find_peaks

ROOT = pathlib.Path(__file__).resolve().parents[1]
SOURCE = pathlib.Path(r'D:\workspace\blog\public\music')
LAME = r'D:\anaconda\pkgs\lame-3.100-hcfcfb64_1003\Library\bin\lame.exe'
TRACKS = [('hekiten', '碧天伴走', 13, 108), ('haruhikage', '春日影 (MyGO!!!!! ver.)', 17, 117),
          ('shiori', '栞', 13, 116), ('mayoiuta', '迷星叫', 15, 108),
          ('hitoshizuku', '壱雫空', 12, 95), ('kageiro', '影色舞', 10, 101)]

def build(track):
    key, title, start, end = track
    out = ROOT / 'Audio' / 'rhythm'
    out.mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='mota-rhythm-') as tmp:
        decoded = pathlib.Path(tmp)/'decoded.wav'
        subprocess.run([LAME, '--silent', '--decode', str(SOURCE/(title+'.mp3')), str(decoded)], check=True, capture_output=True)
        with wave.open(str(decoded)) as w:
            sr, channels = w.getframerate(), w.getnchannels()
            samples = np.frombuffer(w.readframes(w.getnframes()), dtype='<i2').reshape(-1, channels)
        clip = samples[int(start*sr):int(end*sr)].copy()
        # Short fades prevent audible clicks at excerpt boundaries.
        fade = min(sr//5, len(clip)//2)
        clip[:fade] = (clip[:fade]*np.linspace(0,1,fade)[:,None]).astype(np.int16)
        clip[-fade:] = (clip[-fade:]*np.linspace(1,0,fade)[:,None]).astype(np.int16)
        wav = pathlib.Path(tmp)/'clip.wav'
        with wave.open(str(wav), 'wb') as w:
            w.setnchannels(channels); w.setsampwidth(2); w.setframerate(sr); w.writeframes(clip.tobytes())
        subprocess.run([LAME, '--silent', '-b', '160', str(wav), str(out/(key+'.mp3'))], check=True, capture_output=True)
        mono = clip.astype(np.float32).mean(axis=1)[::4]/32768
        rate = sr/4; hop = int(rate*.01); frame = 512
        frames = np.lib.stride_tricks.sliding_window_view(mono,frame)[::hop]
        spec = np.abs(np.fft.rfft(frames*np.hanning(frame), axis=1))
        flux = np.maximum(0, np.diff(np.log1p(spec*5),axis=0)).sum(axis=1)
        peaks, props = find_peaks(flux, distance=18, prominence=max(.5, float(np.std(flux))*.4))
        notes = []
        for i, peak in enumerate(peaks):
            ms = int((peak*hop+frame/2)/rate*1000)
            if 1600 < ms < (end-start)*1000-800:
                lane = [0,1,2,3,2,1,3,0][i%8]
                notes.append([ms,lane])
        print(title, len(notes), 'notes', flush=True)
        return {'id':key, 'title':title, 'start':start, 'end':end,
                'duration':int(len(clip)/sr*1000), 'sectionStatus':'draft-needs-listening', 'notes':notes}

if __name__ == '__main__':
    with concurrent.futures.ThreadPoolExecutor(max_workers=3) as pool:
        tracks = list(pool.map(build, TRACKS))
    (ROOT/'Audio/rhythm/tracks.json').write_text(json.dumps(tracks, ensure_ascii=False), encoding='utf-8')
