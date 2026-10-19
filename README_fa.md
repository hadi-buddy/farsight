<div dir="rtl">

# فارسایت (Farsight) — تبدیل گفتار به متن فارسی برای Google AI Edge Foresight روی مک

[English](README.md) · **فارسی**

**فارسایت** به [Google AI Edge Foresight](https://ai.google.dev/edge)، برنامه‌ی یادداشت‌برداری جلسه‌ی گوگل برای مک، **رونویسی زنده‌ی فارسی** اضافه می‌کند. Foresight فقط انگلیسی می‌فهمد. فارسایت موقع اجرا یک مدل باز فارسی را جای مدل انگلیسی می‌گذارد، تا جلسه‌ها، تماس‌ها و یادداشت‌های صوتی فارسی **کاملاً آفلاین و روی خود مک** به متن تبدیل شوند، بدون هیچ تغییری در خود برنامه.

- 🎙️ **تشخیص گفتار فارسی روی دستگاه** — مدل [شنوا کوچیک](https://huggingface.co/Reza2kn/Shenava-Koochik-v1.0-sherpa-onnx) (۱۱۴ میلیون پارامتر) با [sherpa-onnx](https://github.com/k2-fsa/sherpa-onnx) روی CPU اجرا می‌شود. صدا هیچ‌وقت از مک بیرون نمی‌رود.
- 🔢 **اعداد به رقم** — «هزار و چهارصد و سه» ← «۱۴۰۳».
- 📝 **خلاصه‌ی فارسی جلسه** — خلاصه‌ی جلسه‌های فارسی، فارسی نوشته می‌شود؛ چه با مدل Gemma خود Foresight، چه با یک مدل محلی. تاریخ جلسه هم به شمسی نوشته می‌شود.
- 🔁 **بدون جمله‌ی تکراری** — اگر بدون هدفون صدای بلندگو به میکروفون برسد، جمله‌ی تکراری حذف می‌شود.
- 🔤 **فونت وزیرمتن** — متن فارسی با [وزیرمتن](https://github.com/rastikerdar/vazirmatn) نمایش داده می‌شود، نه فونت پیش‌فرض مک (برای خاموش کردن: `FARSIGHT_FONT=system ./launch.sh`).
- ↔️ **رابط فارسی و راست‌چین** — Foresight ترجمه‌ی فارسی دارد و فارسایت آن را به فارسی باز می‌کند، پس کل برنامه راست‌چین است.
- 🧩 **بدون دستکاری برنامه** — فایل‌های Foresight و امضای گوگل دست‌نخورده می‌مانند. اگر Foresight را معمولی باز کنید، همان نسخه‌ی انگلیسی اصلی است.

> غیررسمی است و ارتباطی با گوگل ندارد. هیچ کد یا مدلی از گوگل در آن نیست.

## در عمل ببینید

</div>

<table>
<tr>
<td width="50%">

<img src="docs/images/meetings-en-ui-fa-summary.jpg" width="100%" alt="بخشِ جلسه‌های Foresight به انگلیسی، با خلاصه‌ای که به فارسی نوشته شده">

<p dir="rtl">رابطِ انگلیسی — خلاصه‌ی هوشمند همچنان کاملاً روان به فارسی نوشته می‌شود</p>

</td>
<td width="50%">

<img src="docs/images/meetings-fa-ui.jpg" width="100%" alt="بخشِ جلسه‌های Foresight، کاملاً فارسی و راست‌چین">

<p dir="rtl">رابطِ فارسی و راست‌چین — پیش‌فرضِ فارسایت</p>

</td>
</tr>
<tr>
<td width="50%">

<img src="docs/images/knowledge-sources-en.jpg" width="100%" alt="بخشِ منابع دانش به انگلیسی">

<p dir="rtl">رابطِ انگلیسی</p>

</td>
<td width="50%">

<img src="docs/images/knowledge-sources-fa.jpg" width="100%" alt="بخشِ منابع دانش به فارسی">

<p dir="rtl">رابطِ فارسی</p>

</td>
</tr>
<tr>
<td width="50%">

<img src="docs/images/stats-en.jpg" width="100%" alt="بخشِ آمار به انگلیسی">

<p dir="rtl">رابطِ انگلیسی</p>

</td>
<td width="50%">

<img src="docs/images/stats-fa.jpg" width="100%" alt="بخشِ آمار به فارسی، با رقم‌های فارسی">

<p dir="rtl">رابطِ فارسی — حتی عددها هم به رقمِ فارسی تبدیل می‌شن</p>

</td>
</tr>
</table>

<div dir="rtl">

## پیش‌نیازها

- مک با تراشه‌ی Apple Silicon
- Foresight نصب‌شده، که یک بار باز شده و دانلود مدل‌هایش تمام شده باشد
- Xcode Command Line Tools — اگر ندارید: `xcode-select --install`
- حدود ۵۰۰ مگابایت فضای خالی (یا ۱۵۰ مگ با مدل کوچک‌تر: `FARSIGHT_MODEL=rizeh ./setup-persian.sh`)

## نصب

این دستورها را به ترتیب در Terminal بزنید:

</div>

```bash
git clone https://github.com/hadi-buddy/farsight.git ~/tools/farsight
cd ~/tools/farsight
./setup-persian.sh
shim/build.sh
./launch.sh
```

<div dir="rtl">

- `setup-persian.sh` موتور فارسی را دانلود می‌کند.
- `shim/build.sh` فارسایت را بیلد می‌کند.
- `launch.sh` برنامه‌ی Foresight را با فارسایت دوباره باز می‌کند.

ریپو باید دقیقاً در `~/tools/farsight` باشد، چون تنظیمات از همان‌جا خوانده می‌شود.

**اختیاری:** اگر Foresight از Dock یا بعد از آپدیت بدون فارسایت باز شد، خودکار با فارسایت دوباره اجرا شود:

</div>

```bash
./install-watcher.sh   # برای حذف: ./install-watcher.sh --uninstall
```

<div dir="rtl">

برای آیکون تو Dock، به‌جای ساختن یه لانچرِ جدا، مستقیم **خودِ Foresight.app** رو پین کن (از `/Applications` بکشش تو Dock، یا موقع اجرا راست‌کلیک → Options → Keep in Dock) — با فعال‌بودنِ `install-watcher.sh`، هر بار که بازش کنی خودکار با شیم اجرا می‌شه. اگه یه اپِ لانچرِ جدا بسازیم، موقعِ اجرا یه آیکنِ جدا (علاوه بر خودِ Foresight) تو Dock می‌مونه — دو تا آیکن برای یه اپ. پین‌کردنِ مستقیمِ Foresight، فقط یکی نگه می‌داره، مثلِ هر اپِ دیگه.

## مطمئن شوید کار می‌کند

</div>

```bash
grep -E "config:|asr" ~/Library/Logs/farsight/shim.log | tail
```

<div dir="rtl">

باید `config: engine=persian` و `asr (persian): ready` را ببینید. یک ضبط شروع کنید و فارسی حرف بزنید. متن بعد از هر مکث، عبارت به عبارت ظاهر می‌شود.

برای عیب‌یابی، با `FARSIGHT_TRACE=1 ./launch.sh` هر نتیجه‌ی رونویسی در لاگ ثبت می‌شود.

## تنظیم دقیق

فایل `tuning.example` را به `~/tools/farsight/tuning` کپی و ویرایش کنید: مکث پایان جمله، حساسیت تشخیص صدا، صدای اضافه قبل و بعد از هر جمله و تعداد هسته‌ی پردازنده. بعد با `./launch.sh` دوباره اجرا کنید؛ نیازی به بیلد نیست.

با `dump = 1` هر جمله به‌صورت فایل WAV همراه با متنش در `~/Library/Logs/farsight/audio/` ذخیره می‌شود تا خطاهای تشخیص را راحت جمع و مقایسه کنید.

## محدودیت‌ها

- **فقط فارسی:** کلمه‌های انگلیسی وسط جمله با حروف فارسی و تقریبی نوشته می‌شوند. برای جلسه‌ی انگلیسی، Foresight را معمولی باز کنید.
- **زبان برنامه:** `launch.sh` برنامه را فارسی و راست‌چین باز می‌کند. برای انگلیسی: `FARSIGHT_LANG=en ./launch.sh`. چند برچسب مثل «Start transcribing» را خود گوگل ترجمه نکرده (چون این‌ها را خودِ Foresight با Flutter رسم می‌کند، نه چیزی که فارسایت بتواند بی‌خطر تغییر بدهد). بعد از اجرای فارسایت، خودِ منوبار Foresight (کنار Window) یک منوی «Language» دارد با فارسی/English — انتخاب کنید و Foresight خودش بدون هیچ فلش و بدون نیاز به ترمینال دوباره با همان زبان باز می‌شود. این انتخاب بعد از بستن کامل و باز کردن عادی (از Dock، Spotlight یا login item) هم می‌ماند، چون به‌جای صرفاً آرگومان خط‌فرمان، مستقیم روی دیسک به‌عنوان یک preference واقعی زیر bundle ID خودِ Foresight نوشته می‌شود — چون Foresight چند ثانیه بعد از هر اجرا خودش را دوباره اجرا می‌کند و آرگومان‌های خودش را از دست می‌دهد.
  <br><img src="docs/images/language-menu.jpg" width="420" alt="منوی Language تو منوبارِ خودِ Foresight، با فارسی و English">
- **عبارت به عبارت:** متن بعد از هر مکث (حدود ۰.۶ ثانیه سکوت) می‌آید، نه کلمه به کلمه.
- **آپدیت Foresight:** ممکن است چیزی را که فارسایت به آن تکیه دارد جابه‌جا کند. اگر بعد از آپدیت متن فارسی ناپدید شد، `shim.log` را ببینید. بدون فارسایت، برنامه به انگلیسی کار می‌کند.

## معماری

Foresight با Flutter ساخته شده و برای رونویسی، یک کتابخانه‌ی C گوگل را صدا می‌زند. مجوزهای برنامه اجازه‌ی تزریق کتابخانه (`DYLD_INSERT_LIBRARIES`) را می‌دهند، پس `launch.sh` برنامه را با `libfarsight.dylib` اجرا می‌کند.

</div>

<img src="docs/images/architecture.jpg" width="100%" alt="دیاگرام: سه نقطه‌ای که libfarsight.dylib داخلِ پروسه‌ی خودِ Foresight قاپ می‌زند — رونویسی، فونت، چت — و هرکدوم به کجا هدایت می‌شود">

<div dir="rtl">

شیم به‌جز مدلِ اختیاریِ چت/خلاصه، برای هیچ‌کدوم از این کارها به یه پروسه‌ی جدا نیاز نداره — دقیقاً داخلِ همون پروسه‌ی خودِ Foresight، فقط آدرسِ تابعی که هر نقطه بهش می‌رسه رو عوض می‌کنه:

۱. **تشخیص گفتار جایگزین می‌شود:** صدایی که Foresight می‌فرستد به ۱۶ کیلوهرتز تبدیل می‌شود، با Silero VAD به عبارت‌ها تقسیم می‌شود، و هر عبارت با مدل شنوا کوچیک به متن فارسی تبدیل می‌شود. نتیجه در همان قالبی برمی‌گردد که Foresight انتظار دارد.

۲. **فیلتر حروف برداشته می‌شود:** Foresight هر حرف غیرانگلیسی را از متن پاک می‌کند. فارسایت این فیلتر را در حافظه عوض می‌کند (فایل روی دیسک دست نمی‌خورد) تا فقط کاراکترهای کنترلی حذف شوند.

۳. **متن فارسی مرتب می‌شود:** اعداد به رقم فارسی تبدیل می‌شوند («یک» تنها به شکل کلمه می‌ماند)، نیم‌فاصله‌ها درست می‌شوند («می‌شه»، «طرح‌های») و «ي/ك» عربی به «ی/ک» فارسی تبدیل می‌شود. هر عبارت با کمی از صدای قبلش به مدل داده می‌شود تا کلمه‌ی اول بریده نشود.

</div>

<img src="docs/images/architecture-shenava-pipeline.jpg" width="100%" alt="دیاگرام: هفت مرحله از صدای خامِ میکروفون تا متنی که تو Foresight ظاهر می‌شه">

<div dir="rtl">

اگر مدل فارسی نباشد یا بالا نیاید، فارسایت به موتور انگلیسی خود Foresight برمی‌گردد.

## چت و خلاصه با مدل محلی

چت و خلاصه‌ی جلسه در Foresight با مدل Gemma خودش اجرا می‌شود که انگلیسی جواب می‌دهد. فارسایت می‌تواند این‌ها را به هر سرور محلی سازگار با OpenAI (مثل llama.cpp یا LM Studio) بفرستد.

بعد از اجرای فارسایت، منوبار خودِ Foresight یک منوی «Reference Model» دارد (کنار Language) — خودش Ollama، LM Studio و یک سرورِ llama.cpp را روی پورت‌های معمولشان پیدا می‌کند و مدل‌های نصب‌شده‌شان را لیست می‌کند، به‌علاوه‌ی گزینه‌ی «Add…» برای هر سرور/مدل دیگر. انتخاب از این منو Foresight را با همان مدل دوباره باز می‌کند، دقیقاً مثل منوی Language — بدون نیاز به ترمینال.

</div>

<img src="docs/images/reference-model-menu.jpg" width="420" alt="منوی Reference Model تو منوبارِ خودِ Foresight، با لیستِ مدل‌های محلیِ شناسایی‌شده">

<img src="docs/images/architecture-menu-sequence.jpg" width="100%" alt="دیاگرامِ Sequence: هاور کردنِ منو سرورهای محلی رو async پراب می‌کنه، انتخابِ یکی config رو می‌نویسه، Foresight بسته می‌شه، و یه helperِ جدا دوباره بازش می‌کنه">

<div dir="rtl">

پشتِ‌پرده، انتخابِ یه مدل فقط یه تنظیم رو عوض نمی‌کنه — انتخاب رو رو دیسک می‌نویسه و Foresight رو کامل می‌بنده و دوباره باز می‌کنه، از طریقِ یه پردازه‌ی کوتاه‌مدتِ جدا (`shim/relaunch.m`)، تا نسخه‌ی جدید config رو تازه بخونه نه این‌که با نسخه‌ی قدیمی مسابقه بده.

مدلِ Gemmaی داخلیِ Foresight کوچیکه و فارسیش قابل‌قبوله ولی یه‌خرده خشک؛ یه مدلِ محلیِ بزرگ‌تر خیلی روان‌تر می‌نویسه:

</div>

<table>
<tr>
<td width="50%">

<img src="docs/images/summary-gemma-default.jpg" width="100%" alt="خلاصه‌ی فارسیِ جلسه با مدلِ پیش‌فرضِ Gemmaی Foresight">

<p dir="rtl">پیش‌فرض — Gemmaی داخلیِ Foresight</p>

</td>
<td width="50%">

<img src="docs/images/summary-qwen-lmstudio.jpg" width="100%" alt="همون خلاصه، این‌بار با مدلِ محلیِ Qwen2.5-7B از طریقِ LM Studio">

<p dir="rtl">مدلِ محلیِ Qwen2.5-7B، از طریقِ LM Studio — فارسیِ به‌مراتب روان‌تر</p>

</td>
</tr>
</table>

<div dir="rtl">

برای سروری روی پورتِ دیگر، یا فیلدهای اضافه‌ای که این منو نمی‌دهد، به‌جایش یک خط در فایل `~/tools/farsight/chat` بنویسید:

</div>

```
<base_url> [model] [{extra JSON request fields}]
```

<div dir="rtl">

یا `off` برای خاموش کردن. برای رونوشت فارسی، از مدل خواسته می‌شود خلاصه را فارسی بنویسد. اگر سرور در دسترس نباشد، دوباره Gemma جواب می‌دهد.

## سپاس

- بر پایه‌ی [foresight-zh](https://github.com/michaelpersonal/foresight-zh) از michaelpersonal، که روش تزریق را برای چینی پیدا کرد.
- مدل‌های گفتار فارسی: [شنوا کوچیک](https://huggingface.co/Reza2kn/Shenava-Koochik-v1.0-sherpa-onnx) و [شنوا ریزه](https://huggingface.co/Reza2kn/Shenava-Rizeh-v1.0-sherpa-onnx) از [Reza2kn](https://github.com/Reza2kn) (Apache-2.0).
- فونت: [وزیرمتن](https://github.com/rastikerdar/vazirmatn) از صابر راستی‌کردار (SIL OFL 1.1).
- [sherpa-onnx](https://github.com/k2-fsa/sherpa-onnx)، [Silero VAD](https://github.com/snakers4/silero-vad)، [cJSON](https://github.com/DaveGamble/cJSON).
- دیاگرام‌های معماری با [diagram-design](https://github.com/cathrynlavery/diagram-design) از Cathryn Lavery ساخته شدن.

</div>
