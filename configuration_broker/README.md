Safe Configuration Management
=============================

All system rely on some form of configuration data, and the configuration interface is a significant part of the attack surface.
Misconfiguration, whether accidential or malicious, is one of the main sources of security vulnerabilites.
The solution in many cases is to create an often complex trust model around and within the configuration system, where the complexity itself adds to the risk profile. 

This example shows how the CHERIoT features can be used to create a system where configuration data can be securely and safely distributed to a number compartments with a simple and minimal trust model.
This supports, for example, a compartement architecture where third party components can be safely integrated and dynamically configured.

There are two main aspects to the security model:
* Static Sealed Capabilities are used to define which compartments can modify and/or consume configuration data, and how much memory each configuration item can consume.  
As well as providing run time controls this aspect of the configuration can be audited at build time.

* A configuration broker provides an abstraction layer between providers and consumers of configuration data, enabling the trust model to be expressed only in terms of the relationships with the broker.

In the context of this example the Configuration Broker is intended to be directly reusable, whilst the providers and consumers are exemplar code of how to develop such compontents using CHERIoT features such as static sealed capabilities, memory claims, locks, event-waiters, and sandbox compartments for handling untrusted data.

Providing a generic broker and expressing the trust model via its interfaces makes it possible to add support for new configurtaion items without having to re-examine the model. 

/// Maybe remove this
In this model a configuration item is a named blob of data.
There are compartments which supply configuration items (for example received by them via the network) and other compartments that need to receive specific configuration items when ever they change.
None of these compartments know about or trust each other; keeping them decoupled helps to keep the system simple, and make it easy to add new items and compartments with a minimal amount of development.
///

## Overview

In the demo each item of configuration data has a name, a value, and a version.

There are three main roles:
* Providers supply the values for one or more items.
* Consumers react when the value of an iterm changes.
* The Broker provides an abstaction layer between Providers and Consumers; it holds the current value and maintains the version for each item.  

The model is similar to a pub/sub architecture with a single retained message for each item and a security policy defined at build time through static sealed capabilities.
The configuration data is declarative so there is no need or value in maintaining a full sequence of updates; each new version is a complete definition of the required configuration item. 
Aligned with the pub/sub model of publishing items and subscribing for items can happen in any sequence; a subscriber will receive any data that is already published, and any subsequent updates.
This avoids any timing issues during system startup.

There is no relationship beween the Provider and Consumers of an item other than a shared understanding of the semantics of the value.

_It is of course possible to add some other relationship, such as a Provider passing sealed values that require the Consumer to know how to ask the Provider for the unsealed value, but that is not the scope or focus of this example._

Providers and Consumers are assigned the right to set or receive each item via Static Sealed Capablities, which means this aspect is fixed at build time and is auditable.

A fourth role is the ability to define a validator for each item, which is called by the broker as part of the update processing.
The ability to define a validator is defined by a separate Static Sealed Capabilty, and only values which pass the validator are made avaliable to Consumers. 

_This is a pragmatic design tradeoff from a zero-trust model where each Consumer is responsible for validation, but which carries the risk that a Consumer may fail to implement this step._
_Having validators registered via a capability means that it is possible to to audit that there is exactly one compartment with the right to define a validator for each item, and to ensure that Consumers only recieve validated data._
_This does however of course mean that the Consumers have to trust the Broker to invoke the validation, and that the validation is correct._
_It also means that three changes are required to add new configuration items, Publisher, Consumer(s), and Validator_

_Having an explicit validator may be a degree of paranoia not required in all cases; It may for example be considered to accept that the publisher code only every supplies valid values._
_In this demo we assume that because a publisher is receiving data from the network it is possible that it can be compromised in some way to make it publish invalid values._
_Validators are registered at startup (to avoid the need to hard code them into the broker) and execute in a sandbox compartment with no capability to 

### Interactions and Trust Model
Becuase the Broker provides an abstration between Providers, Consumers and Validators all of the interactions can be described in terms of thier interactions with the Broker.

#### Providers
Providers have one or more WRITE_CONFIG_CAPABILTY that defines the name and maximum size of each item they are allowed to update. They interact with broker ch  




The example has two sources of configuration data:
* An external system provides values as serialised JSON via a network connection, for example an MQTT client where each value is published to a separate topic.
In the exmple these items are "config1" and "config2" 
* An internal provider which updates the value of "config3" in response to some observable internal event.  




By defining static sealed capabilities we can control at build time:
* Which compartments are allowed to update which items, and the size of items they can supply
* Which compartments are allowed to receive which items

In addition the example shows how memory claims can be used to ensure that heap allocations made in one compartment (broker) remains available to the subscribers outside of the context of the call in which it was delivered.

And finally the example shows how a separate sandpit compartment can be used to perform operations on untrusted data.

## Compartments in the example

### Publisher (Configuration Source)
The **Publisher** compartment provides new values of configuration data.
In a real system this would receive updates from the network, although its possible that some compartments might also need to publish internal configuration updates.
For this example it simply creates new updates to two items, "config1" and "config2" on a periodic basis (for simplicity both items share the same structure).

### Config Broker
The **Config Broker** exposes two cross compartment methods, set_config() to publish and get_config() to receive.
Internally it maintains the most recently published value of each item.
Importantly it has no predefined knowledge of which items, publishers, or subscribers exist, and it's only level of trust is provided by the static sealed capabilities that only it can inspect.
It only makes cross compartment calls to the Allocator and Scheduler; 
A thread calling in from a published will never cross the broker and end up in a subscriber, or vice-versa.

### Subscriber[1-3] (Configuration consumers)
These compartments provide the role of subscribers; 
Subscriber1 is allowed to receive "config1", Subscriber2 is allowed receive "config2", and Subscriber3 is allowed to receive both "config1" and "config2".

### Sandpit
This compartment provides a context in sandpit in which to perform operations on untrusted data; 
It is prevented from making any heap allocations of its own.
This is consistent with the principle that incoming data should be treated as being not only unsafe in content, but potentially even unsafe to parse.
The Broker does not trust the data it receives from the Publisher, and uses the sandpit to avoid any access to the data in its own compartment.
Equally the subscribers do not trust that the data is safe to read until it has been validated in the sandpit. 
The validation used here is very basic, and implemented in a way to deliberately expose it to bound violations.

## Threads in the example
A number of thread are used to provide the dynamic behaviour.
They are described in the context of the compartments where they start and execute their idle loops, although of course they also perform cross compartment calls.

A thread in the Publisher compartment periodically makes updates to both of the configuration items.
A separate thread in Publisher compartment occasionally generates an invalid structure for "config1" which triggers a bounds violation in the validator.
 
Threads in each of the subscribing compartments wait for updates to their configuration items.
The three subscribers each have slightly different behaviour:
    Subscriber#1 waits for notification a change in a single item
    Subscriber#2 waits with a timeout for notification a change in a single item
    Subscriber#3 waits for notification in change to either of two items 

## Use of Sealed Capabilities
Each compartment has a WRITE_CONFIG_CAPABILITY for every value it is allowed to publish and a READ_CONFIG_CAPABILITY for every value it is allowed to receive.

Publishers define their capability with, for example:
```c++
#define CONFIG1 "config1"
DEFINE_WRITE_CONFIG_CAPABILITY(CONFIG1, sizeof(Data))
```
whereas subscribers define theirs with
```c++
#define CONFIG1 "config1"
DEFINE_READ_CONFIG_CAPABILITY(CONFIG1)
```

Note that defining a write capability also requires the maximum size of object that will be published. 

All methods exposed by the Config Broker require the calling compartments capability, which are referenced by name such as
```c++
  WRITE_CONFIG_CAPABILITY(CONFIG1)
```
or
```c++
  READ_CONFIG_CAPABILITY(CONFIG1)
```

## Publishing new values
The Config Broker exposes the following interface to publishers
```c++
/**
 * Set configuration data
 */
int __cheri_compartment("config_broker")
  set_config(SObj configWriteCapability, void *data, size_t size);
```
If size is within the limit defined by the sealed capability the broker will allocate space for the new data and use the Sandbox compartment to copy it.  
If successful it will free any previous space allocated for this item.
The extent of trust is that the Publisher trusts the Broker not to block its thread.
The Publisher does not trust the Broker not to modify its data, and passes in a Read Only capability.
The Broker does not trust the data provided by the Publisher, and will only access the data in the sandpit compartment.
The Broker only allows the Publisher to create a heap allocation defined by its sealed capability.   
 
## Reading configuration values
The Config Broker exposes the following interface to subscribers
```
 /**
 * Read a configuration value.
 */
struct ConfigItem
{
	uint32_t version; // version - used as a futex
	void    *data;    // value
};

ConfigItem *__cheri_compartment("config_broker")
  get_config(SObj configReadCapability);
```
A ConfigItem is returned regardless of whether a value has been published.
If a value has not yet been published then version will be 0 and data will nullptr;
The version element provides a futex that allows the subscriber to wait for changes in value.
For every new value version is incremented and data updated to provide a new non-writable pointer to the new value.

The extent of trust is that the Subscriber trusts the Broker not to block its thread, and to update version
The only value passed from the Subscriber to the Broker is the static sealed capability.
The Subscriber should not trust the contents of the data until it has validated it in the Sandbox compartment. 

The Broker will allocated new heap space and free the existing space when a new value is published.
This can happen at any time, and will happen in advance of subscribers being notified of the change.
Subscribers should place thier own claims on values after verification so that they can continue to access them until it is satisfied that any new value is valid.   


## Memory Management
The Config Broker allocates storage for all config items from its own quota.  The maximum required is defined by the set of static capabilities and can be audited.  

Allocating storage in the Broker and controlling the size via the sealed capabilities avoids the broker from needing any trust in the publisher, which can free or modify its data after it has been used to set the configuration value with no impact.  The copy is performed by the broker after having verified the bounds and allocated new heap space.  If the validation or allocation fails any previous value remains available to subscribers. 

In the example the Publisher allocates a new heap object for each updated, and frees it immediately after the call to set_config() - which is prototypical of something working from a network stack.



## Sandpit Compartment
Although the static sealed capabilities provide protection over who can update and receive configuration items, they can not offer any assurance over the content.
To treat the data as initially untrusted we have to assume that not only may it contain invalid values, but that it may be constructed so as to cause harm when it is being parsed.
In the example the first action of each callback is to pass the received value to a validation method in a sandpit container, which has no access to the callers stack or context and cannot allocate any additional heap.
An error handler in the validator compartment traps any violations, and if the validator does not return a success value the compartment simply ignores the update and keeps its claim on the previous object.

## Running the example
The example is built and run with the normal xmake commands, and has no external dependencies.

Thread priorities are uses to ensure a start up sequence that has subscribed run both before and after the initial data is available.

Debug messages from Publisher show when data is updated, and from each Subscriber when it receives an update.
Debug messages from the Config Broker can be enabled with the configuration option --debug-config_broker=true

Every 12 seconds a malicious value is sent for "config1" which triggers a BoundsViolation in the validator.

Here are some examples of the output

Subscriber #1 reads config1 before any data is available.
```
Config Broker: thread 0x2 get_config called with 0x2004d6c8 (v:1 0x2004d6c8-0x2004d6e4 l:0x1c o:0xc p: G RWcgm- -- ---)
Config Broker: Unsealed id: 0x0 kind: ReadToken(0x0) size:0x0 item: config1
Subscriber #1: thread 0x2 got version:0x0 of config1
Config Data: thread 0x2 config1 -- No config yet
```

Subscriber #2 reads config2 and data is already available.
```
Config Broker: thread 0x3 get_config called with 0x2004d708 (v:1 0x2004d708-0x2004d724 l:0x1c o:0xc p: G RWcgm- -- ---)
Config Broker: Unsealed id: 0x0 kind: ReadToken(0x0) size:0x0 item: config2
Subscriber #2: thread 0x3 got version:0x1 of config2
Config Data: thread 0x3 config2 -- config count: 0x0 token: Coyote
```

Publisher sets a value for item "config1", and Subscriber #1 and Subscriber #3 get the notification and re-read the value.
Note that the Publisher and Subscribers operate on thier own threads.
```
Publisher: thread 0x1 Set config1
Config Broker: thread 0x1 Set config called with 0x2004d670 (v:1 0x2004d670-0x2004d68c l:0x1c o:0xc p: G RWcgm- -- ---) 0x2004dba0 (v:1 0x2004dba0-0x2004dbb8 l:0x18 o:0x0 p: - R----- -- ---) 0x18
Config Broker: Unsealed id: 0x2 kind: WriteToken(0x1) size:0x18 item: config1
Config Broker: Data 0x2004dbc0 (v:1 0x2004dbc0-0x2004dbd8 l:0x18 o:0x0 p: G R-cgm- -- ---)
Config Broker: Waking subscribers 0x3

Config Broker: thread 0x2 get_config called with 0x2004d6c8 (v:1 0x2004d6c8-0x2004d6e4 l:0x1c o:0xc p: G RWcgm- -- ---)
Config Broker: Unsealed id: 0x1 kind: ReadToken(0x0) size:0x0 item: config1
Subscriber #1: thread 0x2 got version:0x3 of config1
Config Data: thread 0x2 config1 -- config count: 0x3 token: Wile-E

Config Broker: thread 0x4 get_config called with 0x2004d748 (v:1 0x2004d748-0x2004d764 l:0x1c o:0xc p: G RWcgm- -- ---)
Config Broker: Unsealed id: 0x5 kind: ReadToken(0x0) size:0x0 item: config1
Subscriber #3: thread 0x4 got version:0x3 of config1
Config Data: thread 0x4 config1 -- config count: 0x3 token: Wile-E
```


Config Source sets a malicious value for "config 1".
Compartment 1 & Compartment 3 read the new value and call into the validator sandpit which traps the error.
The keep the old value, which they still have a claim for. 
```
Publisher: thread 0x5 Sending bad data for config1
Config Broker: thread 0x5 Set config called with 0x2004d670 (v:1 0x2004d670-0x2004d68c l:0x1c o:0xc p: G RWcgm- -- ---) 0x2004dcc0 (v:1 0x2004dcc0-0x2004dcc8 l:0x8 o:0x0 p: G RWcgm- -- ---) 0x4
Config Broker: Unsealed id: 0x2 kind: WriteToken(0x1) size:0x18 item: config1
Config Broker: Data 0x2004dcf0 (v:1 0x2004dcf0-0x2004dcf8 l:0x8 o:0x0 p: G R-cgm- -- ---)
Config Broker: Waking subscribers 0x5

Config Broker: thread 0x2 get_config called with 0x2004d6c8 (v:1 0x2004d6c8-0x2004d6e4 l:0x1c o:0xc p: G RWcgm- -- ---)
Config Broker: Unsealed id: 0x1 kind: ReadToken(0x0) size:0x0 item: config1
Subscriber #1: thread 0x2 got version:0x5 of config1
Sandbox: Detected BoundsViolation(0x1) in Sandbox.  Register CA0(0xa) contained invalid value: 0x2004dcf8 (v:1 0x2004dcf0-0x2004dcf8 l:0x8 o:0x0 p: G R-cgm- -- ---)
Subscriber #1: thread 0x2 Validation failed for 0x2004dcf0 (v:1 0x2004dcf0-0x2004dcf8 l:0x8 o:0x0 p: G R-cgm- -- ---)
Config Data: thread 0x2 config1 -- config count: 0x5 token: Wile-E

Config Broker: thread 0x4 get_config called with 0x2004d748 (v:1 0x2004d748-0x2004d764 l:0x1c o:0xc p: G RWcgm- -- ---)
Config Broker: Unsealed id: 0x5 kind: ReadToken(0x0) size:0x0 item: config1
Subscriber #3: thread 0x4 got version:0x5 of config1
Sandbox: Detected BoundsViolation(0x1) in Sandbox.  Register CA0(0xa) contained invalid value: 0x2004dcf8 (v:1 0x2004dcf0-0x2004dcf8 l:0x8 o:0x0 p: G R-cgm- -- ---)
Subscriber #3: thread 0x4 Validation failed for 0x2004dcf0 (v:1 0x2004dcf0-0x2004dcf8 l:0x8 o:0x0 p: G R-cgm- -- ---)
Config Data: thread 0x4 config1 -- config count: 0x5 token: Wile-E
```

## To Do

Provide example auditing rego for the configuration capabilities, such as max required heap and whether more than one write capability exist for the same item.

Protect the calls into the broker with a futex so we don't try to process more than one request at a time.
That would both limit the extra heap allocation to one item, and protect the changes to the internal vector of items.   

Limit the rate at which changes can be made to a configuration item, to stop a publisher from blitzing the broker.

If two subscribers both have access to the same item then they will both validate it; in some cases this may be OK (they may have different validation requirements), but in others it could be inefficient (although it is always safe).
It's not clear what could track the validated status; the item itself should be immutable (accessed via a read only capability), and the validator stateless.
It might be possible for the validator to call the Config Broker, which could record the status and pass it on to subsequent callbacks - but the complexity of this and the additional attack surface it creates makes me feel its not worth it, esp since shared config values are probably the exception.

