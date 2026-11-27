# MIT License

Copyright (c) 2026 JK Benedict

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

---

## Third-party components

BlitzPad's own source code is under the MIT License above. It is built with these
libraries, which keep their own licences:

| Component | Licence | How BlitzPad uses it |
|---|---|---|
| [QScintilla](https://www.riverbankcomputing.com/software/qscintilla/) (Qt port of Scintilla) | GPL-3.0 (or a commercial licence from Riverbank Computing) | Linked as a shared library from your distribution |
| [Qt 6](https://www.qt.io/) | LGPL-3.0 | Linked as shared libraries from your distribution |
| [7-Zip](https://www.7-zip.org/) / p7zip | LGPL-2.1 (with unRAR restriction) | Run as a separate program for Save Compressed |

Because QScintilla is GPL-3.0, a compiled BlitzPad program, such as `prebuilt/blitzpad`
in the release zips, is a combined work that must be distributed under the terms of the
GPL-3.0. That is allowed, because MIT code may be combined with GPL code. In practice it
means that anyone you give a BlitzPad binary to must also be able to get its source code,
which every release zip and the GitHub repository provide. The MIT licence still applies
to BlitzPad's own source files.
