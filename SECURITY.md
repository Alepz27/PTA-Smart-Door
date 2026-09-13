# Keselamatan dan Kebersihan Repositori Awam

Repositori ini mendokumenkan prototaip kawalan akses bagi tujuan akademik. Kandungannya bukan pensijilan keselamatan atau panduan pemasangan untuk bangunan sebenar.

## Maklumat yang Tidak Boleh Diterbitkan

- SSID dan kata laluan Wi-Fi sebenar
- ID templat atau token pengesahan Blynk
- Token bot atau ID sembang Telegram
- Kunci API, kata laluan, sijil peribadi atau kod pemulihan
- Log akses sebenar atau rekod yang boleh mengenal pasti pengguna
- Tangkap layar papan pemuka atau log bersiri yang belum disemak

Perisian tegar yang diterbitkan menggunakan ruang letak `YOUR_...`. Simpan salinan yang mengandungi maklumat pengesahan sebenar di luar repositori atau dalam fail setempat yang disenaraikan dalam `.gitignore`.

## Semakan Sebelum Menerbitkan Perubahan

1. Semak keseluruhan perbezaan fail, termasuk baris yang dipadam dan fail binari.
2. Cari `password`, `token`, `secret`, `chat_id`, `ssid`, `api_key` dan sebarang nilai sulit yang diketahui.
3. Periksa imej untuk nama, wajah, alamat e-mel, maklumat rangkaian peribadi dan kelayakan sulit.
4. Pastikan arkib yang dijana tidak mengandungi fail sandaran editor atau konfigurasi setempat.
5. Batalkan dan ganti sebarang maklumat pengesahan yang pernah diterbitkan. Memadamkannya dalam komit baharu tidak menyingkirkan nilai tersebut daripada sejarah Git.

## Batasan Keselamatan Prototaip

- Padanan UID RFID tidak tahan terhadap peniruan atau pengklonan.
- Klien Telegram semasa melumpuhkan pengesahan sijil melalui `setInsecure()`.
- Fungsi buka kunci jarak jauh bergantung pada keselamatan akaun dan token yang disambungkan.
- Sistem belum mempunyai penderia kedudukan pintu, pembukaan paksa atau usikan bekas.
- Paparan 10 rekod akses terkini bersifat sementara dan bukan log audit kalis usikan.
- Keperluan elektrik, laluan keluar kecemasan, kebakaran serta pemilihan *fail-safe* atau *fail-secure* mesti dinilai sebelum sebarang pemasangan sebenar.

Laporkan pendedahan maklumat pengesahan secara peribadi dan ganti butiran yang terjejas dengan segera. Jangan buka isu awam yang mengandungi maklumat sulit.
