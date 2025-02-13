/**
 Copyright (c) 2010, Florian Reuter
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without 
 modification, are permitted provided that the following conditions 
 are met:
 
 * Redistributions of source code must retain the above copyright 
 notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright 
 notice, this list of conditions and the following disclaimer in 
 the documentation and/or other materials provided with the 
 distribution.
 * Neither the name of Florian Reuter nor the names of its contributors 
 may be used to endorse or promote products derived from this 
 software without specific prior written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
 COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
 INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
 BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
 STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED 
 OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <opc/mce/textreader.h>
#include <stdio.h>
#ifdef WIN32
#include <crtdbg.h>
#endif


int main( int argc, const char* argv[] )
{
#ifdef WIN32
     _CrtSetDbgFlag (_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    xmlInitParser();
    const char *src=NULL;
    const char *dst=NULL;
    for(int i=1;i<argc;i++) {
        if (0==strcmp("--understands", argv[i])) i++;
        else if (NULL==src) src=argv[i];
        else if (NULL==dst) dst=argv[i];
        else printf("skipped argument %s\n", argv[i]);
    }
    if (NULL!=dst && NULL!=src) {
        mceTextReader_t reader;
        if (-1!=mceTextReaderInit(&reader, xmlNewTextReaderFilename(src))) {
            for(int i=1;i<argc;i++) {
                if (0==strcmp("--understands", argv[i])) {
                    mceTextReaderUnderstandsNamespace(&reader, BAD_CAST(argv[++i]));
                }
            }
            xmlTextWriterPtr writer=xmlNewTextWriterFilename(dst, 0);
            mceTextReaderDump(&reader, writer, false);
            xmlFreeTextWriter(writer);
            mceTextReaderCleanup(&reader);
        } else {
            printf("ERROR: file \"%s\" could not be opened.\n", argv[1]);
        }
    } else {
        printf("mce_read [--understands NAMESPACE] SRC.XML TARGET.XML\n");
    }
    xmlCleanupParser();
    return 0;
}
