/* Copyright (C) 2007-2010 Open Information Security Foundation
 *
 * You can copy, redistribute or modify this Program under the terms of
 * the GNU General Public License version 2 as published by the Free
 * Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 2 along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

/**
 * \file
 *
 * \author Endace Technology Limited - Jason Ish <jason.ish@endace.com>
 *
 * This file provides a basic configuration system for the IDPS
 * engine.
 *
 * NOTE: Setting values should only be done from one thread during
 * engine initialization.  Multiple threads should be able access read
 * configuration data.  Allowing run time changes to the configuration
 * will require some locks.
 *
 * \todo Consider having the in-memory configuration database a direct
 *   reflection of the configuration file and moving command line
 *   parameters to a primary lookup table?
 *
 * \todo Get rid of allow override and go with a simpler first set,
 *   stays approach?
 */

#include <iostream>
#include <cstring>
#include <sys/queue.h>
#include <cinttypes>
#include "conf.h"
#include "util-path.h"
#include "util-strlcpyu.h"
#include "util-strlcatu.h"

using namespace std;

/** Maximum size of a complete domain name. */
#define NODE_NAME_MAX 1024

static ConfNode *root = nullptr;
static ConfNode *root_backup = nullptr;

/**
 * \brief Helper function to get a node, creating it if it does not
 * exist.
 *
 * This function exits on memory failure as creating configuration
 * nodes is usually part of application initialization.
 *
 * \param name The name of the configuration node to get.
 * \param final Flag to set created nodes as final or not.
 *
 * \retval The existing configuration node if it exists, or a newly
 *   created node for the provided name.  On error, nullptr will be returned.
 */
static ConfNode *ConfGetNodeOrCreate(const char *name, int final)
{
    ConfNode *parent = root;
    ConfNode *node = nullptr;
    char node_name[NODE_NAME_MAX];
    char *key;
    char *next;

    if (strlcpy(node_name, name, sizeof(node_name)) >= sizeof(node_name)) {
        printf("Configuration name too long: %s", name);
        return nullptr;
    }

    key = node_name;

    do {
        if ((next = strchr(key, '.')) != nullptr)
            *next++ = '\0';
        if ((node = ConfNodeLookupChild(parent, key)) == nullptr) {
            node = ConfNodeNew();
            if (unlikely(node == nullptr)) {
                printf("Failed to allocate memory for configuration.");
                goto end;
            }
            node->name = strdup(key);
            if (unlikely(node->name == nullptr)) {
                ConfNodeFree(node);
                node = nullptr;
                printf("Failed to allocate memory for configuration.");
                goto end;
            }
            node->parent = parent;
            node->final = final;
            TAILQ_INSERT_TAIL(&parent->head, node, next);
        }
        key = next;
        parent = node;
    } while (next != nullptr);

end:
    return node;
}

/**
 * \brief Initialize the configuration system.
 */
void ConfInit()
{
    if (root != nullptr) {
        printf("already initialized");
        return;
    }
    root = ConfNodeNew();
    if (root == nullptr) {
        printf("ERROR: Failed to allocate memory for root configuration node, "
            "aborting.");
        exit(EXIT_FAILURE);
    }
//    printf("configuration module initialized");
}

/**
 * \brief Allocate a new configuration node.
 *
 * \retval An allocated configuration node on success, nullptr on failure.
 */
ConfNode *ConfNodeNew()
{
    ConfNode *n;

    n = (ConfNode *)calloc(1, sizeof(*n));
    if (unlikely(n == nullptr)) {
        return nullptr;
    }
    TAILQ_INIT(&n->head);

    return n;
}

/**
 * \brief Free a ConfNode and all of its children.
 *
 * \param node The configuration node to SCFree.
 */
void ConfNodeFree(ConfNode *node)
{
    ConfNode *tmp;

    while ((tmp = TAILQ_FIRST(&node->head))) {
        TAILQ_REMOVE(&node->head, tmp, next);
        ConfNodeFree(tmp);
    }

    if (node->name != nullptr)
        free(node->name);
    if (node->val != nullptr)
        free(node->val);
    free(node);
}

/**
 * \brief Get a ConfNode by name.
 *
 * \param name The full name of the configuration node to lookup.
 *
 * \retval A pointer to ConfNode is found or nullptr if the configuration
 *    node does not exist.
 */
ConfNode *ConfGetNode(const char *name)
{
    ConfNode *node = root;
    char node_name[NODE_NAME_MAX];
    char *key;
    char *next;

    if (strlcpy(node_name, name, sizeof(node_name)) >= sizeof(node_name)) {
        printf("Configuration name too long: %s", name);
        return nullptr;
    }

    key = node_name;
    do {
        if ((next = strchr(key, '.')) != nullptr)
            *next++ = '\0';
        node = ConfNodeLookupChild(node, key);
        key = next;
    } while (next != nullptr && node != nullptr);

    return node;
}

/**
 * \brief Get the root configuration node.
 */
ConfNode *ConfGetRootNode()
{
    return root;
}

/**
 * \brief Set a configuration value.
 *
 * Configuration values set with this function may be overridden by
 * subsequent calls, or if the value appears multiple times in a
 * configuration file.
 *
 * \param name The name of the configuration parameter to set.
 * \param val The value of the configuration parameter.
 *
 * \retval 1 if the value was set otherwise 0.
 */
int ConfSet(const char *name, const char *val)
{
    ConfNode *node = ConfGetNodeOrCreate(name, 0);
    if (node == nullptr || node->final) {
        return 0;
    }
    if (node->val != nullptr)
        free(node->val);
    node->val = strdup(val);
    if (unlikely(node->val == nullptr)) {
        return 0;
    }
    return 1;
}

/**
 * \brief Set a configuration parameter from a string.
 *
 * Where the input string is something like:
 *    stream.midstream=true
 *
 * \param input the input string to be parsed.
 *
 * \retval 1 if the value of set, otherwise 0.
 */
int ConfSetFromString(const char *input, int final)
{
    int retval = 0;
    char *name = strdup(input), *val = nullptr;
    if (unlikely(name == nullptr)) {
        goto done;
    }
    val = strchr(name, '=');
    if (val == nullptr) {
        goto done;
    }
    *val++ = '\0';

    while (isspace((int)name[strlen(name) - 1])) {
        name[strlen(name) - 1] = '\0';
    }

    while (isspace((int)*val)) {
        val++;
    }

    if (final) {
        if (!ConfSetFinal(name, val)) {
            goto done;
        }
    }
    else {
        if (!ConfSet(name, val)) {
            goto done;
        }
    }

    retval = 1;
done:
    if (name != nullptr) {
        free(name);
    }
    return retval;
}

/**
 * \brief Set a final configuration value.
 *
 * A final configuration value is a value that cannot be overridden by
 * the configuration file.  Its mainly useful for setting values that
 * are supplied on the command line prior to the configuration file
 * being loaded.  However, a subsequent call to this function can
 * override a previously set value.
 *
 * \param name The name of the configuration parameter to set.
 * \param val The value of the configuration parameter.
 *
 * \retval 1 if the value was set otherwise 0.
 */
int ConfSetFinal(const char *name, char *val)
{
    ConfNode *node = ConfGetNodeOrCreate(name, 1);
    if (node == nullptr) {
        return 0;
    }
    if (node->val != nullptr)
        free(node->val);
    node->val = strdup(val);
    if (unlikely(node->val == nullptr)) {
        return 0;
    }
    node->final = 1;
    return 1;
}

/**
 * \brief Retrieve the value of a configuration node.
 *
 * This function will return the value for a configuration node based
 * on the full name of the node.  It is possible that the value
 * returned could be nullptr, this could happen if the requested node
 * does exist but is not a node that contains a value, but contains
 * children ConfNodes instead.
 *
 * \param name Name of configuration parameter to get.
 * \param vptr Pointer that will be set to the configuration value parameter.
 *   Note that this is just a reference to the actual value, not a copy.
 *
 * \retval 1 will be returned if the name is found, otherwise 0 will
 *   be returned.
 */
int ConfGet(const char *name, char **vptr)
{
    ConfNode *node = ConfGetNode(name);
    if (node == nullptr) {
//        printf("failed to lookup configuration parameter '%s'", name);
        return 0;
    }
    else {
        *vptr = node->val;
        return 1;
    }
}

int ConfGetChildValue(const ConfNode *base, const char *name, char **vptr)
{
    ConfNode *node = ConfNodeLookupChild(base, name);

    if (node == nullptr) {
        printf("failed to lookup configuration parameter '%s'", name);
        return 0;
    }
    else {
        *vptr = node->val;
        return 1;
    }
}


int ConfGetChildValueWithDefault(const ConfNode *base, const ConfNode *dflt,
    const char *name, char **vptr)
{
    int ret = ConfGetChildValue(base, name, vptr);
    /* Get 'default' value */
    if (ret == 0 && dflt) {
        return ConfGetChildValue(dflt, name, vptr);
    }
    return ret;
}

/**
 * \brief Retrieve a configuration value as an integer.
 *
 * \param name Name of configuration parameter to get.
 * \param val Pointer to an intmax_t that will be set the
 * configuration value.
 *
 * \retval 1 will be returned if the name is found and was properly
 * converted to an interger, otherwise 0 will be returned.
 */
int ConfGetInt(const char *name, intmax_t *val)
{
    char *strval;
    intmax_t tmpint;
    char *endptr;

    if (ConfGet(name, &strval) == 0)
        return 0;

    errno = 0;
    tmpint = strtoimax(strval, &endptr, 0);
    if (strval[0] == '\0' || *endptr != '\0')
        return 0;
    if (errno == ERANGE && (tmpint == INTMAX_MAX || tmpint == INTMAX_MIN))
        return 0;

    *val = tmpint;
    return 1;
}

int ConfGetChildValueInt(const ConfNode *base, const char *name, intmax_t *val)
{
    char *strval;
    intmax_t tmpint;
    char *endptr;

    if (ConfGetChildValue(base, name, &strval) == 0)
        return 0;
    errno = 0;
    tmpint = strtoimax(strval, &endptr, 0);
    if (strval[0] == '\0' || *endptr != '\0')
        return 0;
    if (errno == ERANGE && (tmpint == INTMAX_MAX || tmpint == INTMAX_MIN))
        return 0;

    *val = tmpint;
    return 1;

}

int ConfGetChildValueIntWithDefault(const ConfNode *base, const ConfNode *dflt,
    const char *name, intmax_t *val)
{
    int ret = ConfGetChildValueInt(base, name, val);
    /* Get 'default' value */
    if (ret == 0 && dflt) {
        return ConfGetChildValueInt(dflt, name, val);
    }
    return ret;
}


/**
 * \brief Retrieve a configuration value as an boolen.
 *
 * \param name Name of configuration parameter to get.
 * \param val Pointer to an int that will be set to 1 for true, or 0
 * for false.
 *
 * \retval 1 will be returned if the name is found and was properly
 * converted to a boolean, otherwise 0 will be returned.
 */
int ConfGetBool(const char *name, int *val)
{
    char *strval;

    *val = 0;
    if (ConfGet(name, &strval) != 1)
        return 0;

    *val = ConfValIsTrue(strval);

    return 1;
}

int ConfGetChildValueBool(const ConfNode *base, const char *name, int *val)
{
    char *strval;

    *val = 0;
    if (ConfGetChildValue(base, name, &strval) == 0)
        return 0;

    *val = ConfValIsTrue(strval);

    return 1;
}

int ConfGetChildValueBoolWithDefault(const ConfNode *base, const ConfNode *dflt,
    const char *name, int *val)
{
    int ret = ConfGetChildValueBool(base, name, val);
    /* Get 'default' value */
    if (ret == 0 && dflt) {
        return ConfGetChildValueBool(dflt, name, val);
    }
    return ret;
}


/**
 * \brief Check if a value is true.
 *
 * The value is considered true if it is a string with the value of 1,
 * yes, true or on.  The test is not case sensitive, any other value
 * is false.
 *
 * \param val The string to test for a true value.
 *
 * \retval 1 If the value is true, 0 if not.
 */
int ConfValIsTrue(const char *val)
{
    const char *trues[] = {"1", "yes", "true", "on"};
    size_t u;

    for (u = 0; u < sizeof(trues) / sizeof(trues[0]); u++) {
        if (strcasecmp(val, trues[u]) == 0) {
            return 1;
        }
    }

    return 0;
}

/**
 * \brief Check if a value is false.
 *
 * The value is considered false if it is a string with the value of 0,
 * no, false or off.  The test is not case sensitive, any other value
 * is not false.
 *
 * \param val The string to test for a false value.
 *
 * \retval 1 If the value is false, 0 if not.
 */
int ConfValIsFalse(const char *val)
{
    const char *falses[] = {"0", "no", "false", "off"};
    size_t u;

    for (u = 0; u < sizeof(falses) / sizeof(falses[0]); u++) {
        if (strcasecmp(val, falses[u]) == 0) {
            return 1;
        }
    }

    return 0;
}

/**
 * \brief Retrieve a configuration value as a double
 *
 * \param name Name of configuration parameter to get.
 * \param val Pointer to an double that will be set the
 * configuration value.
 *
 * \retval 1 will be returned if the name is found and was properly
 * converted to a double, otherwise 0 will be returned.
 */
int ConfGetDouble(const char *name, double *val)
{
    char *strval;
    double tmpdo;
    char *endptr;

    if (ConfGet(name, &strval) == 0)
        return 0;

    errno = 0;
    tmpdo = strtod(strval, &endptr);
    if (strval[0] == '\0' || *endptr != '\0')
        return 0;
    if (errno == ERANGE)
        return 0;

    *val = tmpdo;
    return 1;
}

/**
 * \brief Retrieve a configuration value as a float
 *
 * \param name Name of configuration parameter to get.
 * \param val Pointer to an float that will be set the
 * configuration value.
 *
 * \retval 1 will be returned if the name is found and was properly
 * converted to a double, otherwise 0 will be returned.
 */
int ConfGetFloat(const char *name, float *val)
{
    char *strval;
    double tmpfl;
    char *endptr;

    if (ConfGet(name, &strval) == 0)
        return 0;

    errno = 0;
    tmpfl = strtof(strval, &endptr);
    if (strval[0] == '\0' || *endptr != '\0')
        return 0;
    if (errno == ERANGE)
        return 0;

    *val = tmpfl;
    return 1;
}

/**
 * \brief Remove (and SCFree) the provided configuration node.
 */
void ConfNodeRemove(ConfNode *node)
{
    if (node->parent != nullptr)
        TAILQ_REMOVE(&node->parent->head, node, next);
    ConfNodeFree(node);
}

/**
 * \brief Remove a configuration parameter from the configuration db.
 *
 * \param name The name of the configuration parameter to remove.
 *
 * \retval Returns 1 if the parameter was removed, otherwise 0 is returned
 *   most likely indicating the parameter was not set.
 */
int ConfRemove(const char *name)
{
    ConfNode *node;

    node = ConfGetNode(name);
    if (node == nullptr)
        return 0;
    else {
        ConfNodeRemove(node);
        return 1;
    }
}

/**
 * \brief Creates a backup of the conf_hash hash_table used by the conf API.
 */
void ConfCreateContextBackup()
{
    root_backup = root;
    root = nullptr;
}

/**
 * \brief Restores the backup of the hash_table present in backup_conf_hash
 *        back to conf_hash.
 */
void ConfRestoreContextBackup()
{
    root = root_backup;
    root_backup = nullptr;
}

/**
 * \brief De-initializes the configuration system.
 */
void ConfDeInit()
{
    if (root != nullptr) {
        ConfNodeFree(root);
        root = nullptr;
    }

//    printf("configuration module de-initialized");
}

static char *ConfPrintNameArray(char **name_arr, int level)
{
    static char name[128*128];
    int i;

    name[0] = '\0';
    for (i = 0; i <= level; i++) {
        strlcat(name, name_arr[i], sizeof(name));
        if (i < level)
            strlcat(name, ".", sizeof(name));
    }

    return name;
}

/**
 * \brief Dump a configuration node and all its children.
 */
void ConfNodeDump(const ConfNode *node, const char *prefix)
{
    ConfNode *child;

    static char *name[128];
    static int level = -1;

    level++;
    TAILQ_FOREACH(child, &node->head, next) {
        name[level] = strdup(child->name);
        if (unlikely(name[level] == nullptr)) {
            continue;
        }
        if (prefix == nullptr) {
            printf("%s = %s\n", ConfPrintNameArray(name, level),
                child->val);
        }
        else {
            printf("%s.%s = %s\n", prefix,
                ConfPrintNameArray(name, level), child->val);
        }
        ConfNodeDump(child, prefix);
        free(name[level]);
    }
    level--;
}

/**
 * \brief Dump configuration to stdout.
 */
void ConfDump()
{
    ConfNodeDump(root, nullptr);
}

/**
 * \brief Lookup a child configuration node by name.
 *
 * Given a ConfNode this function will lookup an immediate child
 * ConfNode by name and return the child ConfNode.
 *
 * \param node The parent configuration node.
 * \param name The name of the child node to lookup.
 *
 * \retval A pointer the child ConfNode if found otherwise nullptr.
 */
ConfNode *ConfNodeLookupChild(const ConfNode *node, const char *name)
{
    ConfNode *child;

    if (node == nullptr || name == nullptr) {
        return nullptr;
    }

    TAILQ_FOREACH(child, &node->head, next) {
        if (child->name != nullptr && strcmp(child->name, name) == 0)
            return child;
    }

    return nullptr;
}

/**
 * \brief Lookup the value of a child configuration node by name.
 *
 * Given a parent ConfNode this function will return the value of a
 * child configuration node by name returning a reference to that
 * value.
 *
 * \param node The parent configuration node.
 * \param name The name of the child node to lookup.
 *
 * \retval A pointer the child ConfNodes value if found otherwise nullptr.
 */
const char *ConfNodeLookupChildValue(const ConfNode *node, const char *name)
{
    ConfNode *child;

    child = ConfNodeLookupChild(node, name);
    if (child != nullptr)
        return child->val;

    return nullptr;
}

/**
 * \brief Lookup for a key value under a specific node
 *
 * \return the ConfNode matching or nullptr
 */

ConfNode *ConfNodeLookupKeyValue(const ConfNode *base, const char *key,
    const char *value)
{
    ConfNode *child;

    TAILQ_FOREACH(child, &base->head, next) {
        if (!strncmp(child->val, key, strlen(child->val))) {
            ConfNode *subchild;
            TAILQ_FOREACH(subchild, &child->head, next) {
                if ((!strcmp(subchild->name, key)) && (!strcmp(subchild->val, value))) {
                    return child;
                }
            }
        }
    }

    return nullptr;
}

/**
 * \brief Test if a configuration node has a true value.
 *
 * \param node The parent configuration node.
 * \param name The name of the child node to test.
 *
 * \retval 1 if the child node has a true value, otherwise 0 is
 *     returned, even if the child node does not exist.
 */
int ConfNodeChildValueIsTrue(const ConfNode *node, const char *key)
{
    const char *val;

    val = ConfNodeLookupChildValue(node, key);

    return val != nullptr ? ConfValIsTrue(val) : 0;
}

/**
 *  \brief Create the path for an include entry
 *  \param file The name of the file
 *  \retval str Pointer to the string path + sig_file
 */
char *ConfLoadCompleteIncludePath(const char *file)
{
    char *defaultpath = nullptr;
    char *path = nullptr;

    /* Path not specified */
    if (Path(file).isRelative()) {
        if (ConfGet("include-path", &defaultpath) == 1) {
            printf("Default path: %s", defaultpath);
            size_t path_len = sizeof(char) * (strlen(defaultpath) +
                          strlen(file) + 2);
            path = (char *)malloc(path_len);
            if (unlikely(path == nullptr))
                return nullptr;
            strlcpy(path, defaultpath, path_len);
            if (path[strlen(path) - 1] != '/')
                strlcat(path, "/", path_len);
            strlcat(path, file, path_len);
       } else {
            path = strdup(file);
            if (unlikely(path == nullptr))
                return nullptr;
        }
    } else {
        path = strdup(file);
        if (unlikely(path == nullptr))
            return nullptr;
    }
    return path;
}

/**
 * \brief Prune a configuration node.
 *
 * Pruning a configuration is similar to freeing, but only fields that
 * may be overridden are, leaving final type parameters.  Additional
 * the value of the provided node is also free'd, but the node itself
 * is left.
 *
 * \param node The configuration node to prune.
 */
void ConfNodePrune(ConfNode *node)
{
    ConfNode *item, *it;

    for (item = TAILQ_FIRST(&node->head); item != nullptr; item = it) {
        it = TAILQ_NEXT(item, next);
        if (!item->final) {
            ConfNodePrune(item);
            if (TAILQ_EMPTY(&item->head)) {
                TAILQ_REMOVE(&node->head, item, next);
                if (item->name != nullptr)
                    free(item->name);
                if (item->val != nullptr)
                    free(item->val);
                free(item);
            }
        }
    }

    if (node->val != nullptr) {
        free(node->val);
        node->val = nullptr;
    }
}

/**
 * \brief Check if a node is a sequence or node.
 *
 * \param node the node to check.
 *
 * \return 1 if node is a seuence, otherwise 0.
 */
int ConfNodeIsSequence(const ConfNode *node)
{
    return node->is_seq == 0 ? 0 : 1;
}

#ifdef UNITTESTS

/**
 * Lookup a non-existant value.
 */
static int ConfTestGetNonExistant(void)
{
    char name[] = "non-existant-value";
    char *value;

    FAIL_IF(ConfGet(name, &value));
    PASS;
}

/**
 * Set then lookup a value.
 */
static int ConfTestSetAndGet(void)
{
    char name[] = "some-name";
    char value[] = "some-value";
    char *value0;

    FAIL_IF(ConfSet(name, value) != 1);
    FAIL_IF(ConfGet(name, &value0) != 1);
    FAIL_IF(strcmp(value, value0) != 0);

    /* Cleanup. */
    ConfRemove(name);

    PASS;
}

/**
 * Test that overriding a value is allowed provided allow_override is
 * true and that the config parameter gets the new value.
 */
static int ConfTestOverrideValue1(void)
{
    char name[] = "some-name";
    char value0[] = "some-value";
    char value1[] = "new-value";
    char *val;

    FAIL_IF(ConfSet(name, value0) != 1);
    FAIL_IF(ConfSet(name, value1) != 1);
    FAIL_IF(ConfGet(name, &val) != 1);
    FAIL_IF(strcmp(val, value1));

    /* Cleanup. */
    ConfRemove(name);

    PASS;
}

/**
 * Test that a final value will not be overrided by a ConfSet.
 */
static int ConfTestOverrideValue2(void)
{
    char name[] = "some-name";
    char value0[] = "some-value";
    char value1[] = "new-value";
    char *val;

    FAIL_IF(ConfSetFinal(name, value0) != 1);
    FAIL_IF(ConfSet(name, value1) != 0);
    FAIL_IF(ConfGet(name, &val) != 1);
    FAIL_IF(strcmp(val, value0));

    /* Cleanup. */
    ConfRemove(name);

    PASS;
}

/**
 * Test retrieving an integer value from the configuration db.
 */
static int ConfTestGetInt(void)
{
    char name[] = "some-int.x";
    intmax_t val;

    FAIL_IF(ConfSet(name, "0") != 1);
    FAIL_IF(ConfGetInt(name, &val) != 1);
    FAIL_IF(val != 0);

    FAIL_IF(ConfSet(name, "-1") != 1);
    FAIL_IF(ConfGetInt(name, &val) != 1);
    FAIL_IF(val != -1);

    FAIL_IF(ConfSet(name, "0xffff") != 1);
    FAIL_IF(ConfGetInt(name, &val) != 1);
    FAIL_IF(val != 0xffff);

    FAIL_IF(ConfSet(name, "not-an-int") != 1);
    FAIL_IF(ConfGetInt(name, &val) != 0);

    PASS;
}

/**
 * Test retrieving a boolean value from the configuration db.
 */
static int ConfTestGetBool(void)
{
    char name[] = "some-bool";
    char *trues[] = {
        "1",
        "on", "ON",
        "yes", "YeS",
        "true", "TRUE",
    };
    char *falses[] = {
        "0",
        "something",
        "off", "OFF",
        "false", "FalSE",
        "no", "NO",
    };
    int val;
    size_t u;

    for (u = 0; u < sizeof(trues) / sizeof(trues[0]); u++) {
        FAIL_IF(ConfSet(name, trues[u]) != 1);
        FAIL_IF(ConfGetBool(name, &val) != 1);
        FAIL_IF(val != 1);
    }

    for (u = 0; u < sizeof(falses) / sizeof(falses[0]); u++) {
        FAIL_IF(ConfSet(name, falses[u]) != 1);
        FAIL_IF(ConfGetBool(name, &val) != 1);
        FAIL_IF(val != 0);
    }

    PASS;
}

static int ConfNodeLookupChildTest(void)
{
    char *test_vals[] = { "one", "two", "three" };
    size_t u;

    ConfNode *parent = ConfNodeNew();
    ConfNode *child;

    for (u = 0; u < sizeof(test_vals)/sizeof(test_vals[0]); u++) {
        child = ConfNodeNew();
        child->name = SCStrdup(test_vals[u]);
        child->val = SCStrdup(test_vals[u]);
        TAILQ_INSERT_TAIL(&parent->head, child, next);
    }

    child = ConfNodeLookupChild(parent, "one");
    FAIL_IF(child == nullptr);
    FAIL_IF(strcmp(child->name, "one") != 0);
    FAIL_IF(strcmp(child->val, "one") != 0);

    child = ConfNodeLookupChild(parent, "two");
    FAIL_IF(child == nullptr);
    FAIL_IF(strcmp(child->name, "two") != 0);
    FAIL_IF(strcmp(child->val, "two") != 0);

    child = ConfNodeLookupChild(parent, "three");
    FAIL_IF(child == nullptr);
    FAIL_IF(strcmp(child->name, "three") != 0);
    FAIL_IF(strcmp(child->val, "three") != 0);

    child = ConfNodeLookupChild(parent, "four");
    FAIL_IF(child != nullptr);

    FAIL_IF(ConfNodeLookupChild(nullptr, nullptr) != nullptr);

    if (parent != nullptr) {
        ConfNodeFree(parent);
    }

    PASS;
}

static int ConfNodeLookupChildValueTest(void)
{
    char *test_vals[] = { "one", "two", "three" };
    size_t u;

    ConfNode *parent = ConfNodeNew();
    ConfNode *child;
    const char *value;

    for (u = 0; u < sizeof(test_vals)/sizeof(test_vals[0]); u++) {
        child = ConfNodeNew();
        child->name = SCStrdup(test_vals[u]);
        child->val = SCStrdup(test_vals[u]);
        TAILQ_INSERT_TAIL(&parent->head, child, next);
    }

    value = (char *)ConfNodeLookupChildValue(parent, "one");
    FAIL_IF(value == nullptr);
    FAIL_IF(strcmp(value, "one") != 0);

    value = (char *)ConfNodeLookupChildValue(parent, "two");
    FAIL_IF(value == nullptr);
    FAIL_IF(strcmp(value, "two") != 0);

    value = (char *)ConfNodeLookupChildValue(parent, "three");
    FAIL_IF(value == nullptr);
    FAIL_IF(strcmp(value, "three") != 0);

    value = (char *)ConfNodeLookupChildValue(parent, "four");
    FAIL_IF(value != nullptr);

    ConfNodeFree(parent);

    PASS;
}

static int ConfGetChildValueWithDefaultTest(void)
{
    char  *val = "";
    ConfCreateContextBackup();
    ConfInit();
    ConfSet("af-packet.0.interface", "eth0");
    ConfSet("af-packet.1.interface", "default");
    ConfSet("af-packet.1.cluster-type", "cluster_cpu");

    ConfNode *root = ConfGetNode("af-packet.0");
    ConfNode *dflt = ConfGetNode("af-packet.1");
    ConfGetChildValueWithDefault(root, dflt, "cluster-type", &val);
    FAIL_IF(strcmp(val, "cluster_cpu"));

    ConfSet("af-packet.0.cluster-type", "cluster_flow");
    ConfGetChildValueWithDefault(root, dflt, "cluster-type", &val);

    FAIL_IF(strcmp(val, "cluster_flow"));

    ConfDeInit();
    ConfRestoreContextBackup();
    PASS;
}

static int ConfGetChildValueIntWithDefaultTest(void)
{
    intmax_t val = 0;
    ConfCreateContextBackup();
    ConfInit();
    ConfSet("af-packet.0.interface", "eth0");
    ConfSet("af-packet.1.interface", "default");
    ConfSet("af-packet.1.threads", "2");

    ConfNode *root = ConfGetNode("af-packet.0");
    ConfNode *dflt = ConfGetNode("af-packet.1");
    ConfGetChildValueIntWithDefault(root, dflt, "threads", &val);
    FAIL_IF(val != 2);

    ConfSet("af-packet.0.threads", "1");
    ConfGetChildValueIntWithDefault(root, dflt, "threads", &val);
    FAIL_IF(val != 1);

    ConfDeInit();
    ConfRestoreContextBackup();

    PASS;
}

static int ConfGetChildValueBoolWithDefaultTest(void)
{
    int val;
    ConfCreateContextBackup();
    ConfInit();
    ConfSet("af-packet.0.interface", "eth0");
    ConfSet("af-packet.1.interface", "default");
    ConfSet("af-packet.1.use-mmap", "yes");

    ConfNode *root = ConfGetNode("af-packet.0");
    ConfNode *dflt = ConfGetNode("af-packet.1");
    ConfGetChildValueBoolWithDefault(root, dflt, "use-mmap", &val);
    FAIL_IF(val == 0);

    ConfSet("af-packet.0.use-mmap", "no");
    ConfGetChildValueBoolWithDefault(root, dflt, "use-mmap", &val);
    FAIL_IF(val);

    ConfDeInit();
    ConfRestoreContextBackup();

    PASS;
}

/**
 * Test the removal of a configuration node.
 */
static int ConfNodeRemoveTest(void)
{
    ConfCreateContextBackup();
    ConfInit();

    FAIL_IF(ConfSet("some.nested.parameter", "blah") != 1);

    ConfNode *node = ConfGetNode("some.nested.parameter");
    FAIL_IF(node == nullptr);
    ConfNodeRemove(node);

    node = ConfGetNode("some.nested.parameter");
    FAIL_IF(node != nullptr);

    ConfDeInit();
    ConfRestoreContextBackup();

    PASS;
}

static int ConfSetTest(void)
{
    ConfCreateContextBackup();
    ConfInit();

    /* Set some value with 2 levels. */
    FAIL_IF(ConfSet("one.two", "three") != 1);
    ConfNode *n = ConfGetNode("one.two");
    FAIL_IF(n == nullptr);

    /* Set another 2 level parameter with the same first level, this
     * used to trigger a bug that caused the second level of the name
     * to become a first level node. */
    FAIL_IF(ConfSet("one.three", "four") != 1);

    n = ConfGetNode("one.three");
    FAIL_IF(n == nullptr);

    /* A top level node of "three" should not exist. */
    n = ConfGetNode("three");
    FAIL_IF(n != nullptr);

    ConfDeInit();
    ConfRestoreContextBackup();

    PASS;
}

static int ConfGetNodeOrCreateTest(void)
{
    ConfNode *node;

    ConfCreateContextBackup();
    ConfInit();

    /* Get a node that should not exist, give it a value, re-get it
     * and make sure the second time it returns the existing node. */
    node = ConfGetNodeOrCreate("node0", 0);
    FAIL_IF(node == nullptr);
    FAIL_IF(node->parent == nullptr || node->parent != root);
    FAIL_IF(node->val != nullptr);
    node->val = SCStrdup("node0");
    node = ConfGetNodeOrCreate("node0", 0);
    FAIL_IF(node == nullptr);
    FAIL_IF(node->val == nullptr);
    FAIL_IF(strcmp(node->val, "node0") != 0);

    /* Do the same, but for something deeply nested. */
    node = ConfGetNodeOrCreate("parent.child.grandchild", 0);
    FAIL_IF(node == nullptr);
    FAIL_IF(node->parent == nullptr || node->parent == root);
    FAIL_IF(node->val != nullptr);
    node->val = SCStrdup("parent.child.grandchild");
    node = ConfGetNodeOrCreate("parent.child.grandchild", 0);
    FAIL_IF(node == nullptr);
    FAIL_IF(node->val == nullptr);
    FAIL_IF(strcmp(node->val, "parent.child.grandchild") != 0);

    /* Test that 2 child nodes have the same root. */
    ConfNode *child1 = ConfGetNodeOrCreate("parent.kids.child1", 0);
    ConfNode *child2 = ConfGetNodeOrCreate("parent.kids.child2", 0);
    FAIL_IF(child1 == nullptr || child2 == nullptr);
    FAIL_IF(child1->parent != child2->parent);
    FAIL_IF(strcmp(child1->parent->name, "kids") != 0);

    ConfDeInit();
    ConfRestoreContextBackup();

    PASS;
}

static int ConfNodePruneTest(void)
{
    ConfNode *node;

    ConfCreateContextBackup();
    ConfInit();

    /* Test that final nodes exist after a prune. */
    FAIL_IF(ConfSet("node.notfinal", "notfinal") != 1);
    FAIL_IF(ConfSetFinal("node.final", "final") != 1);
    FAIL_IF(ConfGetNode("node.notfinal") == nullptr);
    FAIL_IF(ConfGetNode("node.final") == nullptr);
    FAIL_IF((node = ConfGetNode("node")) == nullptr);
    ConfNodePrune(node);
    FAIL_IF(ConfGetNode("node.notfinal") != nullptr);
    FAIL_IF(ConfGetNode("node.final") == nullptr);

    /* Test that everything under a final node exists after a prune. */
    FAIL_IF(ConfSet("node.final.one", "one") != 1);
    FAIL_IF(ConfSet("node.final.two", "two") != 1);
    ConfNodePrune(node);
    FAIL_IF(ConfNodeLookupChild(node, "final") == nullptr);
    FAIL_IF(ConfGetNode("node.final.one") == nullptr);
    FAIL_IF(ConfGetNode("node.final.two") == nullptr);

    ConfDeInit();
    ConfRestoreContextBackup();

    PASS;
}

int ConfNodeIsSequenceTest(void)
{
    ConfNode *node = ConfNodeNew();
    FAIL_IF(node == nullptr);
    FAIL_IF(ConfNodeIsSequence(node));
    node->is_seq = 1;
    FAIL_IF(!ConfNodeIsSequence(node));

    if (node != nullptr) {
        ConfNodeFree(node);
    }
    PASS;
}

static int ConfSetFromStringTest(void)
{
    ConfNode *n;

    ConfCreateContextBackup();
    ConfInit();

    FAIL_IF_NOT(ConfSetFromString("stream.midstream=true", 0));
    n = ConfGetNode("stream.midstream");
    FAIL_IF_NULL(n);
    FAIL_IF_NULL(n->val);
    FAIL_IF(strcmp("true", n->val));

    FAIL_IF_NOT(ConfSetFromString("stream.midstream =false", 0));
    n = ConfGetNode("stream.midstream");
    FAIL_IF_NULL(n);
    FAIL_IF(n->val == nullptr || strcmp("false", n->val));

    FAIL_IF_NOT(ConfSetFromString("stream.midstream= true", 0));
    n = ConfGetNode("stream.midstream");
    FAIL_IF_NULL(n);
    FAIL_IF(n->val == nullptr || strcmp("true", n->val));

    FAIL_IF_NOT(ConfSetFromString("stream.midstream = false", 0));
    n = ConfGetNode("stream.midstream");
    FAIL_IF_NULL(n);
    FAIL_IF(n->val == nullptr || strcmp("false", n->val));

    ConfDeInit();
    ConfRestoreContextBackup();
    PASS;
}

void ConfRegisterTests(void)
{
    UtRegisterTest("ConfTestGetNonExistant", ConfTestGetNonExistant);
    UtRegisterTest("ConfSetTest", ConfSetTest);
    UtRegisterTest("ConfTestSetAndGet", ConfTestSetAndGet);
    UtRegisterTest("ConfTestOverrideValue1", ConfTestOverrideValue1);
    UtRegisterTest("ConfTestOverrideValue2", ConfTestOverrideValue2);
    UtRegisterTest("ConfTestGetInt", ConfTestGetInt);
    UtRegisterTest("ConfTestGetBool", ConfTestGetBool);
    UtRegisterTest("ConfNodeLookupChildTest", ConfNodeLookupChildTest);
    UtRegisterTest("ConfNodeLookupChildValueTest",
                   ConfNodeLookupChildValueTest);
    UtRegisterTest("ConfNodeRemoveTest", ConfNodeRemoveTest);
    UtRegisterTest("ConfGetChildValueWithDefaultTest",
                   ConfGetChildValueWithDefaultTest);
    UtRegisterTest("ConfGetChildValueIntWithDefaultTest",
                   ConfGetChildValueIntWithDefaultTest);
    UtRegisterTest("ConfGetChildValueBoolWithDefaultTest",
                   ConfGetChildValueBoolWithDefaultTest);
    UtRegisterTest("ConfGetNodeOrCreateTest", ConfGetNodeOrCreateTest);
    UtRegisterTest("ConfNodePruneTest", ConfNodePruneTest);
    UtRegisterTest("ConfNodeIsSequenceTest", ConfNodeIsSequenceTest);
    UtRegisterTest("ConfSetFromStringTest", ConfSetFromStringTest);
}

#endif /* UNITTESTS */
