/*
 * Copyright (c) 2014 Citrix Systems, Inc.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* USB policy daemon
 * Policy handling module
 */

#include "project.h"


/* Strings we expect to see in the XML */

#define XML_POLICY  "policy"  /* Top level XML node */
#define XML_RULE    "match"   /* Each rule in the XML */
#define XML_ACTION  "action"  /* Rule action */
#define XML_ALLOW   "allow"   /* Allow action value */
#define XML_DENY    "deny"    /* Deny action value */

#define USB_FIELD_CLASS        0  /* class value */
#define USB_FIELD_SUBCLASS     1  /* subclass value */
#define USB_FIELD_PROTOCOL     2  /* protocol value */
#define USB_FIELD_VID          3  /* vendor ID value */
#define USB_FIELD_PID          4  /* product ID value */
#define USB_FIELD_FOCUS        5  /* is domain in focus */
#define USB_FIELD_MAX          6

static const struct {
    char *name;  /* Name, as it appears in the XML */
    int bits;    /* Max size of legal value, in bits */
} fieldSpec[USB_FIELD_MAX] = {
    { "class",     8 },
    { "subclass",  8 },
    { "protocol",  8 },
    { "VID",       16 },
    { "PID",       16 },
    { "focus",     1 }
};

static const char *defaultXML = "<policy> <match action=\"allow\" /> </policy>";
static const char *UIVMuuid = "00000000-0000-0000-0000-000000000002";


/* Policy storage types */

typedef struct {
    int field;
    int value;
} RuleCriteria;

typedef struct {
    int criteriaCount;       /* Number of criteria to match in rule */
    int criteriaSpace;       /* Number of criteria we've allocated space for */
    RuleCriteria *criteria;  /* Criteria to match */
    int allow;               /* Does rule allow (1) or deny (0) access */
} PolicyRule;

typedef struct _Policy {
    int ruleCount;           /* Number of rules in policy */
    int ruleSpace;           /* Number fo rules we've allocated space for */
    PolicyRule *rules;       /* Rules */
    char *uuid;              /* UUID of domain policy applies to. <0 => error, ignore this policy */
    int isUIVM;              /* Is the domain this policy applies to the UIVM */
} Policy;

static Policy *policies = NULL;  /* Policies to match devices against, one per domain */
static int policyCount = 0;      /* Number of domains we have policies for */



/* Functions to access policy storage in database */

static Buffer db_path = { NULL, 0 };
//static Buffer uuid_buf = { NULL, 0 };
static Buffer xml_buffer = { NULL, 0 };

#define POLICY_DB_PATH  "/com/citrix/xenclient/ctxusb_policy/"


/* Fetch policy for the given domain from the database into the xml_buffer
 * param    domuuid              uuid of domain to fetch policy for
 * return                        pointer to xml_buffer containg policy on success, NULL on failure
 */
const char *fetchPolicy(const char *domuuid)
{
    int i;
    int r;

    /* Set up database path */
    setBuffer(&db_path, POLICY_DB_PATH);
    catToBuffer(&db_path, domuuid);

    bufferSize(&xml_buffer, 100);  /* Setup initial buffer */

    /* Get policy XML from database */
    while(1)
    {
        r = xcdbus_read_db(rpc_xcbus(), db_path.data, xml_buffer.data, xml_buffer.len);
        
        if(r == 0 || xml_buffer.data[0] == '\0')
        {
            /* No policy found */
            LogInfo("No USB policy found for domain %s", domuuid);
            return NULL;
        }

        for(i = 0; i < xml_buffer.len; ++i)
        {
            if(xml_buffer.data[i] == '\0')
            {
                /* Policy retrieved */
                LogInfo("Retrieved USB policy for domain %s:", domuuid);
                LogInfo("%s", xml_buffer.data);
                return xml_buffer.data;
            }
        }

        /* Policy may be bigger than buffer, enlarge and retry */
        bufferSize(&xml_buffer, xml_buffer.len * 2);
    }
}


/* Write the given policy to the database
 * param    domuuid              uuid of domain to write policy for
 * param    policy               XML policy for domain
 * return                        0 on success, non-0 on failure
 */
int storePolicy(const char *domuuid, const char *policy)
{
    static IBuffer devs = { NULL, 0, 0 };
    int i, err;

    /* Set up database path */
    setBuffer(&db_path, POLICY_DB_PATH);
    catToBuffer(&db_path, domuuid);

    /* Write policy XML to database */
    if(policy[0] == '\0')
    {
        LogInfo("Clearing USB policy for domain %s", domuuid);
    }
    else
    {
        LogInfo("Storing USB policy for domain %s:", domuuid);
        LogInfo("%s", policy);
    }

    /* Dump our policy cache */
    deletePolicies();

    if (!xcdbus_write_db(rpc_xcbus(), db_path.data, policy)) {
        LogError("Error writing policy for domain %s to database", domuuid);
        return -1;
    }

    /* clear sticky devices if blocked by policy for this domain */
    if (domuuid) {
        getDeviceList(&devs);
        for (i = 0; i < devs.len; ++i) {
            int devid = devs.data[i];
            clearStickyIfPolicyBlocked( devid, domuuid );
        }
        clearIBuffer(&devs);
    }
    return 0;
}



/* Functions to parse and store policies */

/* Add the specified criteria to the given rule
 * param    rule                 rule to add criteria to
 * param    field                field to test
 * param    value                text form of value to test field for
 * return                        0 on success, non-0 on failure
 */
static int addCriteria(PolicyRule *rule, int field, const char *value)
{
    int len;          /* Required criteria list length */
    unsigned long v;  /* Numeric form of field value */
    char *suf;        /* Value suffix */

    assert(rule != NULL);
    assert(field >= 0 && field < USB_FIELD_MAX);

    len = rule->criteriaCount + 1;

    if(len > rule->criteriaSpace)
    {
        /* Need more space */
        len += 4;  /* Add some extra space to reduce reallocs */
        rule->criteria = realloc(rule->criteria, len * sizeof(RuleCriteria));
        if(rule->criteria == NULL)
        {
            LogError("Couldn't allocate memory for policy");
            exit(2);
        }

        rule->criteriaSpace = len;
    }

    rule->criteria[rule->criteriaCount].field = field;
    rule->criteria[rule->criteriaCount].value = 0;
    rule->criteriaCount++;

    /* Convert value from string to text */
    suf = NULL;
    v = strtoul(value, &suf, 16);
    
    assert(suf != NULL);
    if(*suf != '\0' || v >= (1 << fieldSpec[field].bits))
    {
        /* Unexpected suffix */
        LogError("Policy XML error: invalid value \"%s\" for %s", value, fieldSpec[field].name);
        return -1;
    }
    
    rule->criteria[rule->criteriaCount - 1].value = v;
    return 0;
}


/* Add a rule to the given policy
 * param    policy               policy to add rule to
 * return                        new rule, NULL on failure
 */
static PolicyRule *addRule(Policy *policy)
{
    PolicyRule *rule;  /* New rule */
    int len;           /* Required rule list length */

    assert(policy != NULL);

    len = policy->ruleCount + 1;

    if(len > policy->ruleSpace)
    {
        /* Need more space */
        len += 4;  /* Add some extra space to reduce reallocs */
        policy->rules = realloc(policy->rules, len * sizeof(PolicyRule));
        if(policy->rules == NULL)
        {
            LogError("Couldn't allocate memory for policy");
            exit(2);
        }

        policy->ruleSpace = len;
    }

    rule = &(policy->rules[policy->ruleCount]);
    rule->allow = -1;
    rule->criteriaCount = 0;
    rule->criteriaSpace = 0;
    rule->criteria = NULL;
    policy->ruleCount++;

    return rule;
}


/* Delete all the policies in our list, if any */
void deletePolicies(void)
{
    int i, r;

    for(i = 0; i < policyCount; ++i)
    {
        if(policies[i].rules != NULL)
        {
            /* Delete the rules in policy i */
            for(r = 0; r < policies[i].ruleCount; ++r)
            {
                /* For each rule we just need to delete the criteria */
                if(policies[i].rules[r].criteria != NULL)
                    free(policies[i].rules[r].criteria);
            }

            if(policies[i].rules != NULL)
                free(policies[i].rules);
        }
    }

    if(policies != NULL)
        free(policies);

    policies = NULL;
    policyCount = 0;
}


/* Parse the given criteria, which are provided as a list of strings, with each pair being
 * field, value pairs. The list is terminated with a single NULL string.
 * param    rule                 rule to add parsed criteria to
 * param    criteria             list of criteria strings
 * return                        0 on success, non-0 on failure
 */
static int parseCriteria(PolicyRule *rule, const char *criteria[])
{
    int i;  /* Criteria counter */
    int j;  /* Known field names counter */
    int k;  /* Existing criteria counter */

    assert(rule != NULL);
    assert(criteria != NULL);

    for(i = 0; criteria[i] != NULL; i += 2)
    {
        /* Look for specified field amongst our known list */
        for(j = 0; j < USB_FIELD_MAX; ++j)
        {
            if(strcmp(criteria[i], fieldSpec[j].name) == 0)
            {
                /* we've found the field */
                for(k = 0; k < rule->criteriaCount; ++k)
                {
                    if(rule->criteria[k].field == j)
                    {
                        LogError("Policy XML error: field %s specified more than once in single rule", criteria[i]);
                        break;
                    }
                }

                if(k == rule->criteriaCount)
                    if(addCriteria(rule, j, criteria[i + 1]) != 0)
                        return -1;

                break;
            }
        }

        if(strcmp(criteria[i], XML_ACTION) == 0)
        {
            if(rule->allow != -1)
                LogError("Policy XML error: rule contains multiple action specifiers");
            else if(strcmp(criteria[i + 1], XML_ALLOW) == 0)
                rule->allow = 1;
            else if(strcmp(criteria[i + 1], XML_DENY) == 0)
                rule->allow = 0;
            else
                LogError("Policy XML error: unrecognised action specifier \"%s\"", criteria[i + 1]);
        }
        else if(j > USB_FIELD_MAX)
        {
            LogError("Policy XML error: could not resolve rule field %s", criteria[i]);
        }
    }

    return 0;
}        


/* Print out our policy
 * param    buffer               buffer to write state into
 */
void dumpPolicy(Buffer *buffer)
{
    int i;
    int r;
    int c;

    assert(buffer != NULL);

    if(policyCount == 0)
    {
        catToBuffer(buffer, "No policies loaded\n");
        return;
    }

    catToBuffer(buffer, "Policies (%d):\n", policyCount);

    for(i = 0; i < policyCount; ++i)
    {
        catToBuffer(buffer, "  uuid %s\n", policies[i].uuid);

        for(r = 0; r < policies[i].ruleCount; ++r)
        {
            catToBuffer(buffer, "    ");

            if(policies[i].rules[r].allow == 1)
                catToBuffer(buffer, "allow");
            else
                catToBuffer(buffer, "deny ");

            catToBuffer(buffer, ":");

            if(policies[i].rules[r].criteriaCount == 0)
            {
                catToBuffer(buffer, " all");
            }

            for(c = 0; c < policies[i].rules[r].criteriaCount; ++c)
            {
                assert(policies[i].rules[r].criteria[c].field >= 0 && policies[i].rules[r].criteria[c].field < USB_FIELD_MAX);
                catToBuffer(buffer, " %s=%x", fieldSpec[policies[i].rules[r].criteria[c].field].name, policies[i].rules[r].criteria[c].value);
            }

            catToBuffer(buffer, "\n");
        }
    }
}



/* Interface with Expat */

/* We use global data to record our position within the XML pasre tree */
static Policy *xpat_policy;    /* Policy being loaded */
static int xpat_policy_done;   /* Have we finished loading the policy yet, 0 => not yet, 1 => loading, 2 => loaded */
static PolicyRule *xpat_rule;  /* Current rule being loaded, NULL for none */
static int xpat_error;         /* Has a policy parsing error occurred */

/* Called by Expat when an element is started */
static void xpat_start(void *data, const char *el, const char **attr)
{
    assert(el != NULL);
    assert(attr != NULL);

    if(xpat_error != 0)  /* It's already gone wrong, do nothing */
        return;

    if(xpat_policy_done > 1)
    {
        /* We should already be finished */
        LogError("Policy XML error: unexpected trailing node \"%s\"", el);
        xpat_error = 1;
        return;
    }

    if(xpat_policy_done == 0)
    {
        /* We've had nothing yet, expect a policy */
        if(strcmp(el, XML_POLICY) != 0)
        {
            LogError("Policy XML error: expected " XML_POLICY " node, got \"%s\"", el);
            xpat_error = 1;
        }

        xpat_policy_done = 1;
        return;
    }

    if(xpat_rule != NULL)
    {
        /* We're in a rule, didn't expect any sub elements */
        LogError("Policy XML error: unexpected node \"%s\" in " XML_RULE, el);
        xpat_error = 1;
        return;
    }

    /* This should be a rule node */
    if(strcmp(el, XML_RULE) != 0)
    {
        LogError("Policy XML error: expected " XML_RULE " node, got \"%s\"", el);
        xpat_error = 1;
        return;
    }

    /* Create new rule */
    xpat_rule = addRule(xpat_policy);
    assert(xpat_rule != NULL);

    /* Handle rule specification */
    if(parseCriteria(xpat_rule, attr) != 0)
        xpat_error = 1;
}


/* Called by Expat when an element is ended */
static void xpat_end(void *data, const char *el)
{
    assert(el != NULL);

    if(xpat_error != 0)  /* It's already gone wrong, do nothing */
        return;

    if(strcmp(el, XML_POLICY) == 0)  /* This is the end of the policy */
        xpat_policy_done = 2;

    if(strcmp(el, XML_RULE) == 0)  /* This is the end of the rule */
        xpat_rule = NULL;
}


/* Set the given policy to that described by the given XML
 * param    policy               policy
 * param    xml                  XML describing policy
 */
static void setDomainPolicy(Policy *policy, const char *xml)
{
    XML_Parser p;
    assert(policy != NULL);
    assert(xml != NULL);

    /* Set up globals used to interface with Expat */
    xpat_policy = policy;
    xpat_policy_done = 0;
    xpat_rule = NULL;
    xpat_error = 0;

    /* Call Expat */
    p = XML_ParserCreate(NULL);
    if(p == NULL)
    {
        LogError("Couldn't allocate memory for XML parser");
        exit(2);
    }

    XML_SetElementHandler(p, xpat_start, xpat_end);

    if(!XML_Parse(p, xml, strlen(xml), 1))
    {
        LogError("Policy XML error: %s", XML_ErrorString(XML_GetErrorCode(p)));
        xpat_error = 1;
    }

    if(xpat_error != 0)
    {
        free(policy->uuid);
        policy->uuid = NULL;
    }
}

/* Find the policy for the specified domain
 * param    uuid                 UUID of domain to find policy for
 * return                        domain policy
 */
static Policy *getPolicy(const char *uuid)
{
    int i;

    assert(uuid != NULL);

    /* Check if we already have an appropriate policy */
    for(i = 0; i < policyCount; ++i)
        if(strcmp(policies[i].uuid, uuid) == 0)  /* match */
            return &policies[i];

    /* No policy found for this domain, load one */
    i = policyCount;
    ++policyCount;
    policies = realloc(policies, sizeof(Policy) * policyCount);
    if(policies == NULL)
    {
        LogError("Could not allocate USB policy space (%d bytes)",
                 sizeof(Policy) * policyCount);
        exit(2);
    }

    policies[i].ruleCount = 0;
    policies[i].ruleSpace = 0;
    policies[i].rules = NULL;
    policies[i].uuid = strcopy(uuid);
    policies[i].isUIVM = 0;

    /* Get policy XML from database */
    if(fetchPolicy(uuid) == NULL)
    {
        LogInfo("Could not find policy for domain %s using default", uuid);
        setBuffer(&xml_buffer, defaultXML);
    }

    if(strcmp(uuid, UIVMuuid) == 0)
        policies[i].isUIVM = 1;

    setDomainPolicy(&policies[i], xml_buffer.data);
    return &policies[i];
}



/* Functions to check policy */

/* Compare the given interface info against the given rule
 * param    info                 info of interface to check rule for
 * param    rule                 rule to check against
 * return                        1 if rule matches, 0 otherwise
 */
static int compareRuleIface(int info[USB_FIELD_MAX], PolicyRule *rule)
{
    int i;

    assert(info != NULL);
    assert(rule != NULL);

    /* Check criteria one at a time. Note that criteria are anded for a match */
    for(i = 0; i < rule->criteriaCount; ++i)
    {
        assert(rule->criteria[i].field >= 0 && rule->criteria[i].field < USB_FIELD_MAX);
        if(info[rule->criteria[i].field] != rule->criteria[i].value)  /* no match */
            return 0;
    }

    /* all criteria match */
    return 1;
}


/* Compare the given interface info against the given policy
 * param    info                 info of interface to check policy for
 * param    focus                id of domain currently in focus
 * param    policy               policy to check against
 * return                        0 to deny VM access to this device, 1 to allow
 */
static int comparePolicyIface(int info[USB_FIELD_MAX], int focus, Policy *policy)
{
    int rule;

    assert(info != NULL);
    assert(policy != NULL);

    /* Ignore hubs (class 9) */
    if(info[USB_FIELD_CLASS] == 9)
        return 0;

    /* Is this domain in focus? */
    info[USB_FIELD_FOCUS] = 1; //(focus == policy->domid) ? 1 : 0;

    /* Check rules one at a time */
    for(rule = 0; rule < policy->ruleCount; ++rule)
    {
        if(compareRuleIface(info, &(policy->rules[rule])) != 0)
        {
            /* match */
            assert(policy->rules[rule].allow == 0 || policy->rules[rule].allow == 1);
            return policy->rules[rule].allow;
        }
    }

    /* No rules match, default is to deny everything */
    return 0;
}


/* Check the given USB classes against the specified policy
 * param    uuid                 uuid of policy to check against
 * param    Vid                  Vid of device being checked
 * param    Pid                  Pid of device being checked
 * param    class_count          number of classes to compare to policy
 * param    classes              array of classes to compare to policy
 * return                        non-0 if policy allows class list, 0 otherwise
 */
int consultPolicy(const char *uuid, int Vid, int Pid, int class_count, int *classes)
{
    int info[USB_FIELD_MAX];
    Policy *p;
    int i;

    info[USB_FIELD_PROTOCOL] = 0;  /* Not currently used */
    info[USB_FIELD_VID] = Vid;
    info[USB_FIELD_PID] = Pid;
    info[USB_FIELD_FOCUS] = 1;  /* Not currently used */

    p = getPolicy(uuid);

    for(i = 0; i < class_count; ++i)
    {
        info[USB_FIELD_CLASS] = (classes[i] >> 8) & 0xFF;
        info[USB_FIELD_SUBCLASS] = classes[i] & 0xFF;

        if(comparePolicyIface(info, 0, p) != 0)
        {
            /* Device allowed */
            return 1;
        }
    }

    /* Not allowed */
    return 0;
}
