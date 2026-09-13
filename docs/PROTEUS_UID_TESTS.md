# Kes Ujian UID RFID

## Tujuan

Rujukan ini merekodkan nilai RFID yang digunakan untuk menyemak simulasi Smart Door dalam Proteus. Masukkan setiap UID dalam *Virtual Terminal* tepat seperti yang ditunjukkan.

## Matriks Ujian

| UID RFID | Status kebenaran | Keputusan dijangka |
| --- | --- | --- |
| `E280689401A9` | Dibenarkan | Akses dibenarkan; nama ALI dipaparkan; pintu dibuka |
| `E2000019060C` | Dibenarkan | Akses dibenarkan; nama ABU dipaparkan; pintu dibuka |
| `123456789ABC` | Tidak dibenarkan | Akses ditolak; pintu kekal berkunci; sistem kembali ke keadaan sedia |

## Cara Menggunakan Rujukan Ini

1. Jalankan simulasi Proteus.
2. Salin satu UID daripada jadual tanpa teks lain.
3. Tampalkan UID ke dalam *Virtual Terminal*.
4. Tekan **Enter** dan bandingkan respons yang diperhatikan dengan keputusan dijangka.
5. Catat keputusan sebenar secara berasingan; jadual di atas bukan rekod keputusan ujian yang telah dilaksanakan.

Lihat [Panduan Simulasi Smart Door Menggunakan Proteus](PROTEUS_SIMULATION_GUIDE.md) untuk arahan lengkap.
