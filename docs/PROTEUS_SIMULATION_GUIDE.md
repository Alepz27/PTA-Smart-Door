# Panduan Simulasi Smart Door Menggunakan Proteus

## Tujuan

Dokumen ini menerangkan cara menjalankan dan menyemak simulasi kawalan akses RFID yang disertakan dalam projek PTA Smart Door. Panduan ini disediakan untuk demonstrasi makmal dan penilaian akademik.

## Fail dan Perisian yang Diperlukan

- [Projek Proteus: Litar simulasi Smart Door.pdsprj](Litar%20simulasi%20Smart%20Door.pdsprj)
- [Kes ujian UID RFID](PROTEUS_UID_TESTS.md)
- Proteus 8 atau versi serasi yang boleh membuka fail `.pdsprj`

## Prosedur

### 1. Muat turun projek simulasi

1. Buka fail `.pdsprj` yang dipautkan di atas.
2. Pilih **Raw** atau **Download raw file** untuk menyimpan fail pada komputer.
3. Kekalkan nama asal dan sambungan `.pdsprj`.
4. Buka fail yang dimuat turun menggunakan Proteus.

### 2. Jalankan simulasi

1. Pastikan skema litar dipaparkan.
2. Tekan butang **Run** (ikon main).
3. Perhatikan LCD dan *Virtual Terminal*. LCD sepatutnya menunjukkan keadaan sedia menerima kad, manakala *Virtual Terminal* digunakan untuk memasukkan UID RFID.

### 3. Uji akses yang dibenarkan

1. Buka [kes ujian UID RFID](PROTEUS_UID_TESTS.md).
2. Salin UID sahaja tanpa nama pengguna.
3. Klik dalam *Virtual Terminal*, tampalkan UID dan tekan **Enter**.
4. Gunakan `E280689401A9` bagi pengguna **ALI** atau `E2000019060C` bagi pengguna **ABU**.
5. Pastikan LCD menunjukkan akses dibenarkan dan output akses menunjukkan pintu dibuka.

### 4. Uji akses yang tidak dibenarkan

1. Masukkan `123456789ABC` dalam *Virtual Terminal*.
2. Tekan **Enter**.
3. Pastikan pintu kekal berkunci dan LCD kembali ke keadaan sedia menerima kad.

## Keputusan Dijangka

| Input ujian | Respons LCD dan sistem yang dijangka |
| --- | --- |
| `E280689401A9` | Akses dibenarkan; pengguna dikenal pasti sebagai ALI; pintu dibuka |
| `E2000019060C` | Akses dibenarkan; pengguna dikenal pasti sebagai ABU; pintu dibuka |
| `123456789ABC` | Akses ditolak; pintu kekal berkunci; sistem kembali ke keadaan sedia |

Keputusan dijangka ialah kriteria semakan, bukan bukti bahawa ujian telah dilaksanakan. Penilai perlu merekodkan pemerhatian sebenar bagi setiap kes.

## Skop Simulasi

Simulasi memodelkan urutan asas kawalan akses RFID: *Virtual Terminal* menerima UID, Arduino membandingkannya dengan senarai yang dibenarkan, kemudian sistem menerima atau menolak akses. LCD, LED, pembaz dan output geganti memberikan maklum balas yang boleh diperhatikan.

Simulasi ini tidak mewakili keseluruhan perkakasan ESP32, penderia AS608 atau perkhidmatan awan dalam prototaip bersepadu.

## Penyelesaian Masalah

- Klik dalam *Virtual Terminal* sebelum menampal UID.
- Jalankan simulasi sebelum memasukkan nilai ujian.
- Tekan **Enter** selepas setiap UID.
- Masukkan UID perenambelasan sahaja tanpa nama pengguna atau ruang tambahan.
- Jika fail dibuka sebagai teks dalam pelayar, muat turun fail tersebut dan bukanya menggunakan Proteus.
