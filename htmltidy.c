#include <stdio.h>
#include <tidy.h>
#include <tidybuffio.h>
#include <curl/curl.h>

int iso_8859_1s_to_utf8s(char *utf8str, const char *mbstr, size_t count)
{
    int res = 0;

    for (; *mbstr != '\0'; ++mbstr) {
        /* the character needs no mapping if the highest bit is not set */
        if ((*mbstr & 0x80) == 0) {
            if (utf8str != 0 && count != 0) {
                /* check space left in the destination buffer */
                if (res >= count) {
                    return res;
                }

                *utf8str++ = *mbstr;
            }
            ++res;
        }
        /* otherwise mapping is necessary */
        else {
            if (utf8str != 0 && count != 0) {
                /* check space left in the destination buffer */
                if (res+1 >= count) {
                    return res;
                }

                *utf8str++ = (0xC0 | (0x03 & (*mbstr >> 6)));
                *utf8str++ = (0x80 | (0x3F & *mbstr));
            }
            res += 2;
        }
    }

    /* add the terminating null character */
    if (utf8str != 0 && count != 0) {
        /* check space left in the destination buffer */
        if (res >= count) {
            return res;
        }
        *utf8str = 0;
    }

    return res;
}

/* curl write callback, to fill tidy's input buffer...  */ 
uint write_cb(char *in, uint size, uint nmemb, TidyBuffer *out)
{
    uint r;
    r = size * nmemb;

    char *bufenc;
    bufenc = malloc(r);
    int rc = iso_8859_1s_to_utf8s(bufenc, in, r);
    printf("%s\n", bufenc);
    tidyBufAppend(out, bufenc, r);
    return r;
}

/* Traverse the document tree */ 
void dumpNode(TidyDoc doc, TidyNode tnod)
{
    TidyNode child;
    for(child = tidyGetChild(tnod); child; child = tidyGetNext(child) ) {
        ctmbstr name = tidyNodeGetName(child);
        if(name) {
            /* if it has a name, then it's an HTML tag ... */ 
            TidyAttr attr;
            //printf("%s\n", name);
            /* walk the attribute list */ 
            for(attr=tidyAttrFirst(child); attr; attr=tidyAttrNext(attr) ) {
                //printf(tidyAttrName(attr));
                /*tidyAttrValue(attr)?printf("=\"%s\" ",
                        tidyAttrValue(attr)):printf(" ");*/
            }
            //printf(">\n");
        }
        else {
            /* if it doesn't have a name, then it's probably text, cdata, etc... */ 
            TidyBuffer buf;
            tidyBufInit(&buf);
            tidyNodeGetText(doc, child, &buf);
            printf("%s\n", buf.bp?(char *)buf.bp:"");
            tidyBufFree(&buf);
        }
        dumpNode(doc, child); /* recursive */ 
    }
}


int main(int argc, char **argv)
{
    CURL *curl;
    char curl_errbuf[CURL_ERROR_SIZE];
    char *url = "http://mto38.free.fr/frog";
    TidyDoc tdoc;
    TidyBuffer docbuf = {0};
    TidyBuffer tidy_errbuf = {0};
    int err;
    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_errbuf);
    // curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);

    tdoc = tidyCreate();
    tidyOptSetBool(tdoc, TidyForceOutput, yes); /* try harder */ 
    tidyOptSetInt(tdoc, TidyWrapLen, 4096);
    tidySetErrorBuffer(tdoc, &tidy_errbuf);
    tidySetCharEncoding(tdoc, "UTF8");
    tidyBufInit(&docbuf);

    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &docbuf);
    err=curl_easy_perform(curl);
    if(!err) {
        err = tidyParseBuffer(tdoc, &docbuf); /* parse the input */ 
        if(err >= 0) {
            err = tidyCleanAndRepair(tdoc); /* fix any problems */ 
            if(err >= 0) {
                err = tidyRunDiagnostics(tdoc); /* load tidy error buffer */ 
                if(err >= 0) {
                    dumpNode(tdoc, tidyGetRoot(tdoc)); /* walk the tree */ 
                    // fprintf(stderr, "%s\n", tidy_errbuf.bp); /* show errors */ 
                }
            }
        }
    }
    /* clean-up */ 
    curl_easy_cleanup(curl);
    tidyBufFree(&docbuf);
    tidyBufFree(&tidy_errbuf);
    tidyRelease(tdoc);
    return err;
}
