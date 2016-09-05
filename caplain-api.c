#include <stdio.h>
#include <tidy.h>
#include <tidybuffio.h>
#include <curl/curl.h>

void print_node(TidyDoc doc, TidyNode tnod) 
{
    TidyBuffer buf;
    tidyBufInit(&buf);
    tidyNodeGetText(doc, tnod, &buf);
    printf("%s\n", buf.bp?(char *)buf.bp:""); 
    tidyBufFree(&buf);
}

void handle_print_class(TidyDoc doc, TidyNode tnod, TidyAttr attr) 
{
    TidyNode prt_nod;
    if (tidyAttrValue(attr) == NULL) {
        return;
    }

    if (!strcmp(tidyAttrValue(attr), "titre")) {
        printf("\n");        
    }

    if (!strcmp(tidyAttrValue(attr), "titre")
            || !strcmp(tidyAttrValue(attr), "donnees") 
            || !strcmp(tidyAttrValue(attr), "blabla")) {

        prt_nod = tidyGetChild(tnod);
        print_node(doc, prt_nod);
    }
}

void handle_print_tags(TidyDoc doc, TidyNode tnod, ctmbstr name) 
{
    TidyNode prt_nod, prt2_nod;

    if (!strcmp(name, "title")) {
        printf("\n");
        prt_nod = tidyGetChild(tnod);
        print_node(doc, prt_nod);
    }

    if (!strcmp(name, "i")) {
        printf("\n");
        prt_nod = tidyGetChild(tnod);
        print_node(doc, prt_nod);
    }
}

/* Traverse the document tree */ 
void process_tree(TidyDoc doc, TidyNode tnod)
{
    TidyNode child;
    for(child = tidyGetChild(tnod); child; child = tidyGetNext(child) ) {
        ctmbstr name = tidyNodeGetName(child);
        if(name) {
            /* if it has a name, then it's an HTML tag ... */ 
            TidyAttr attr;
            handle_print_tags(doc, child, name);       
            
            /* walk the attribute list */ 
            for(attr=tidyAttrFirst(child); attr; attr=tidyAttrNext(attr)) {
                handle_print_class(doc, child, attr);       
            }
        }
        else {
            /* if it doesn't have a name, then it's probably text, cdata, etc... */ 
            // print_node(doc, child);
        }
        process_tree(doc, child); /* recursive */ 
    }
}

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
    //printf("%s\n", bufenc);
    tidyBufAppend(out, bufenc, r);
    free(bufenc);
    return r;
}

int main(int argc, char **argv)
{
    CURL *curl;
    char curl_errbuf[CURL_ERROR_SIZE];
    char *url = "http://mto38.free.fr";
    TidyDoc tdoc;
    TidyBuffer docbuf = {0};
    TidyBuffer tidy_errbuf = {0};
    int err;
    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_errbuf);
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
                    process_tree(tdoc, tidyGetRoot(tdoc)); /* walk the tree */ 
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
