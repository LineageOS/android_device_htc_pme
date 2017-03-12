/*
 * Copyright (C) 2017, The LineageOS Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "operator-properties"
//#define LOG_NDEBUG 0

#include <stdio.h>
#include <stdlib.h>
#include <cutils/log.h>
#include <cutils/properties.h>
#include <tinyxml.h>

#define MNS_MAP_XML "/system/customize/mns_map.xml"

static const char *find_filename(TiXmlNode *node, unsigned op)
{
    const char *last_match = NULL;

    for (TiXmlNode *c = node->FirstChild(); c != NULL; c = c->NextSibling()) {
        if (c->Type() == TiXmlNode::ELEMENT && c->Value() != NULL) {
            if (strcmp(c->Value(), "item") == 0) {
                const char *filename = NULL;
                bool match = false;
                TiXmlElement *e = (TiXmlElement *) c;

                for (TiXmlAttribute *attrib = e->FirstAttribute(); attrib != NULL; attrib = attrib->Next()) {
                    const char *name = attrib->Name();
                    const char *value = attrib->Value();

                    if (name == NULL) {
                        ALOGE("missing name");
                    } else if (value == NULL) {
                        ALOGE("missing value for attribute %s", name);
                    } else if (strcmp(name, "name") == 0 && atoi(value) == op) {
                        match = true;
                    } else if (strcmp(name, "CNmapfile") == 0) {
                        filename = value;
                    }
                }
                if (match) {
                    last_match = filename;
                }
            } else {
                const char *match = find_filename(c, op);
                if (match) last_match = match;
            }
        }
    }

    return last_match;
}

static char buf[1024*1024];

static void load_properties(const char *filename)
{
    FILE *f;

    if ((f = fopen(filename, "r")) == NULL) {
        ALOGI("Failed to open %s", filename);
        exit(1);
    }

    while (fgets(buf, sizeof(buf), f) != NULL) {
        char *key = buf;

        while (isspace(*key)) key++;
        if (*key == '#') continue;
        char *eol = key+strlen(key)-1;
        while (eol > key && isspace(*eol)) *eol-- = '\0';
        char *value = strchr(key, '=');
        if (value) {
            *value++ = '\0';
            while (isspace(*value)) value++;
            if (property_set(key, value) < 0) {
                ALOGE("failed to set property [%s] = [%s]", key, value);
            }
        }
    }

    fclose(f);
}

int main(int argc, char **argv)
{
    unsigned op;
    const char *filename;
    char full_filename[256];
    char bootcid[PROPERTY_VALUE_MAX];

    property_get("ro.boot.cid", bootcid, "");
    if (strcmp(bootcid, "BS_US001") != 0 && strcmp(bootcid, "BS_US002") != 0) {
        ALOGI("not an unlocked variant");
        exit(0);
    }

    op = property_get_int32("gsm.sim.operator.numeric", 0);

    if (op == 0) {
        ALOGI("no operator defined");
        exit(0);
    }

    ALOGI("operator-properties for operator %d", op);

    TiXmlDocument doc(MNS_MAP_XML);
    if (! doc.LoadFile()) {
        ALOGE("Failed to load %s", MNS_MAP_XML);
        exit(1);
    }

    filename = find_filename(&doc, op);
    if (filename == NULL) filename = "default.prop";

    sprintf(full_filename, "/system/customize/MNSprop/%s", filename);
    ALOGI("Using properties %s", full_filename);

    load_properties(full_filename);

    return 0;
}
