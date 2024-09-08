# easy-dbuspp

Modern C++ D-Bus wrapper.

## Introduction

The library has two major goals:

* Use regular, run-of-the-mill modern C++ and its standard types to abstract away almost all of D-Bus.
* Keep usage as simple as possible.

If a missing feature conflicts with these two goals, it will stay missing.

***Please note that this is still very much work in progress. Until version 1, the code may change
considerably. API stability is not yet guaranteed.***

## Example usage

The library provides the `easydbuspp.h` convenience header, which brings in everything you need to start
using it.

Here's how we create a service:

```cpp
easydbuspp::session_manager session_manager {easydbuspp::bus_type_t::SESSION, "net.test.EasyDBuspp.Test"};
easydbuspp::object object {session_manager, "net.test.EasyDBuspp.TestInterface", "/net/test/EasyDBuspp/TestObject"};
```

The second parameter to `session_manager`'s constructor is the bus name we're requesting.
Then we create an object which uses the `session_manager` instance, with a given interface
name and a D-Bus object path.

### Registering methods

Registering a method with the D-Bus `object` is as simple as using `object::add_method()` with the
method name and a callable entity (lambda, functor object, regular function pointer).

```cpp
object.add_method("MethodTakingAStringAndReturningBool",
                  [](const std::string& input) {
                      return input == "password";
                  });
```

That's it! Now whenever something calls `MethodTakingAStringAndReturningBool` via D-Bus, the
library automatically invokes your lambda and does all the legwork of packing the returned
value and sending it back to the caller. You don't have to know anyting else!

### Reporting errors

Errors are simply reported by throwing an `std::exception`-derived exception. For example, say
we don't want to accept empty strings as input to `MethodTakingAStringAndReturningBool`:

```cpp
object.add_method("MethodTakingAStringAndReturningBool",
                  [](const std::string& input) {
                      if (input.empty())
                          throw std::runtime_error("No empty input strings allowed!");
                      return input == "password";
                  });
```

Again, that's it! The exception will simply be picked up by the library and converted into an
error that's sent back, via the D-Bus wire, to the caller!

### Returning multiple values

Just return an `std::tuple` (or `std::pair` if you prefer).

```cpp
const std::tuple<int, std::string, double> EXPECTED_VALUE {42, "Life, the Universe and Everything", 3.14};

object.add_method("ReturnMultipleValues", [&EXPECTED_VALUE]() {
    return EXPECTED_VALUE;
});
```

That will create a method with three output parameters (in D-Bus parlance):

```
ReturnMultipleValues(out i out_arg0,
                     out s out_arg1,
                     out d out_arg2);
```

### Adding properties

D-Bus' properties can be read-only, read-write, and read-only. The library will figure out
what type a property is based on the type of the value provided (literals, `const`s, will be
read-only properties automatically, while lvalues will be read-write properties
automatically).

```cpp
// Read-only property, because 42 is an int literal.
object.add_property("TheAnswerToLifeTheUniverseAndEverything", 42);

const double PI {3.1415};
// Read-only property, because add_property() sees a const value.
object.add_property("ReadOnlyDoubleProp", PI);

// Read only property, because this decays to a const char*.
object.add_property("GreatestPhilosopher", "Ludwig Wittgenstein");

// Read-write property, because readwrite_str is seen as an lvalue.
std::string readwrite_str {"This value can be modified"};
object.add_property("ReadWriteStringProp", readwrite_str);
```

Properties can also be registered by providing setter and getter callbacks.
If only the getter callback is provided, the property will be read-only.
If both are provided, the property will be readwrite. And if only the setter
is provided, the property will be write-only.

```cpp
std::vector<std::string> free_jazz_musicians {"Albert Ayler", "Peter Brötzmann"};

// Read/write property, because we're passing both a setter and a getter.
object.add_property<std::vector<std::string>>(
    "FreeJazzMusicians",
    [&free_jazz_musicians] {
        return free_jazz_musicians;
    },
    [&free_jazz_musicians](const std::vector<std::string>& new_value) {
        free_jazz_musicians = new_value;
        return true;
    });
```

We can make that into a write-only property like this:

```cpp
// Write-only property.
object.add_property<std::vector<std::string>>(
    "FreeJazzMusicians",
    {}, // No read (getter) function provided.
    [&free_jazz_musicians](const std::vector<std::string>& new_value) {
        free_jazz_musicians = new_value;
        return true;
    });
```

### Registering signals

Signals are similar to UDP packets. They belong to an interface, and can contain data (parameters).
Emitting a signal is similar to calling a void "function" with parameters that are supposed to
be sent out somewhere, but it may be received or not. If it's lost, it's lost (just like an UDP
packet).

The most common, and useful, type of signal is a broadcast signal. A broadcast signal is sent out
to anyone who'll listen, on any bus.

Here's how to add one to your object:

```cpp
auto broadcast_signal = object.add_broadcast_signal<int, std::string, float>("BroadcastSignal");

object.add_method("TriggerBroadcastSignal", [&broadcast_signal] {
    broadcast_signal(42, "Broadcast signal emitted!", 3.14f);
});
```

The first line creates a broadcast signal named `"BroadcastSignal"`, taking parameters of type
`int, std::string, float`. `add_broadcast_signal()` returns an
`std::function<void(int, std::string, float>`, which is then used inside the new
`"TriggerBroadcastSignal"` to emit the signal.

So now every time `"TriggerBroadcastSignal"` is called, the signal gets sent out.

### Starting the service

Now that our object has been set up, we can make it available by starting the main processing loop:

```cpp
session_manager.run();
```

This plugs `SIGTERM` and `SIGINT` in, so doing `^C` in your terminal will shut down gracefully.

If you want to start the service asynchronously, that's possible too:

```cpp
session_manager.run_async();
```

And that's it! You can introspect and use your shiny new D-Bus object with one of a variety of
command line or GUI tools:

* [d-spy](https://apps.gnome.org/Dspy/)
* [qtdbusviewer](https://doc.qt.io/qt-6/qdbusviewer.html)
* [gdbus](https://manpages.ubuntu.com/manpages/focal/man1/gdbus.1.html)

### D-Bus parameter names

Let's introspect our object with `gdbus` (ignoring the properties for the purposes of this section):

```
gdbus introspect --session -d net.test.EasyDBuspp.Test -o /net/test/EasyDBuspp/TestObject
[...]
  interface net.test.EasyDBuspp.TestInterface {
    methods:
      MethodTakingAStringAndReturningBool(in  s in_arg0,
                                          out b out_arg0);
      TriggerBroadcastSignal();
    signals:
      BroadcastSignal(i arg0,
                      s arg1,
                      d arg2);
    properties:
  };
```

Notice that the introspected parameter names have been automatically generated (`in_arg0`, `out_arg0` for
the method, `arg0`, `arg1`, `arg2` for the signal).

What if you want to name them, though? That's possible by specifying (optional) `std::vector<std::string>` parameters
for input parameter names, and output parameter names (C++ callable return type(s)):

```cpp
object.add_method("MethodTakingAStringAndReturningBool",
                  [](const std::string& input) {
                      return input == "password";
                  }, {"input"}, {"authorization_succeeded"});

```

The same goes for signals:

```cpp
auto broadcast_signal = object.add_broadcast_signal<int, std::string, float>(
    "BroadcastSignal", {"int_param", "string_param", "float_param"});
```

Introspecting the object again shows that it's working:

```
interface net.test.EasyDBuspp.TestInterface {
  methods:
    MethodTakingAStringAndReturningBool(in  s input,
                                        out b authorization_succeeded);
    TriggerBroadcastSignal();
  signals:
    BroadcastSignal(i int_param,
                    s string_param,
                    d float_param);
  properties:
};
```

### Proxies: the other side of the coin

Now that we have a service up and running, we can connect to it using `easydbuspp::proxy`.

Here's how to set one up:

```cpp
easydbuspp::session_manager session_manager {easydbuspp::bus_type_t::SESSION};
easydbuspp::proxy           proxy(session_manager, "net.test.EasyDBuspp.Test",
                                  "net.test.EasyDBuspp.TestInterface",
                                  "/net/test/EasyDBuspp/TestObject");
```

### Calling a method

Calling methods is, well, easy. Just specify the return type you're expecting as the template
parameter to `proxy::call()`, then the method name as the first parameter, followed by all the
parameters the function expects:

```cpp
// Result will be a bool.
auto result = proxy.call<bool>("MethodTakingAStringAndReturningBool", "password");
```

### Handling errors

Remember our method threw an `std::runtime_error` if it's parameter was an empty string?
Well, that is magically turned into an `std::runtime_error` on the proxy side, so all we
have to do to handle errors is enclose our `call()` in a `try / catch` block:

```cpp
try {
    auto result = proxy.call<bool>("MethodTakingAStringAndReturningBool", "");
} catch (const std::exception& e) {
    cerr << "Error: " << e.what() << "\n";
}
```

### Working with properties

The proxy caches object properties upon initial connection. Reading a property thus
retrieves it from the cache, which is convenient since it presupposes no expensive I/O:

```cpp
auto philosopher = proxy.cached_property<std::string>("GreatestPhilosopher");
```

The cache stays current for all remote objects that trigger the `"PropertiesChanged"`
broadcast signal (which is almost everyone).

We can set a property in our own cache (but this won't affect the remote object) like
this:

```cpp
proxy.cached_property("ReadWriteStringProp", "New value");
```

If we _do_ want to affect the remote object, there's no way around an expensive proxy call
to `org.freedesktop.DBus.Properties.Set`, but we can do that too:

```cpp
proxy.property("ReadWriteStringProp", "Value to set on remote object");
```

Reading the property directly from the remote object is possible as well, with the same
caveats as above (it will do a method call):

```cpp
auto string_prop = proxy.property<std::string>("ReadWriteStringProp");
```

In other words, the `cached_`-prefix versions will try to work with the local version, the
other versions will work directly with the remote object.

### Registering for a signal

In order to be able to passively receive signals, an application needs to start running the
main processing loop. But before that, it should register for the signal, which, by now
unsurprisingly, it can do by registering a callback:

```cpp
session_manager.signal_subscribe("BroadcastSignal", [&session_manager](int i, const std::string& s, float f) {
    std::cout << "Got signal 'BroadcastSignal': [" << i << ", '" << s << "', " << f << "]\nExiting." << std::endl;
    session_manager.stop();
});
```

Then we could just run the main processing loop in a different thread:

```cpp
session_manager.run_async();
```

leaving the main thread free to trigger the signal:

```cpp
proxy.call<void>("TriggerBroadcastSignal");
```

Once the signal is triggered, the callback is called, which stops the main thread loop,
which allows the process to exit.

### Passing D-Bus context to methods

This library deliberately makes a big effort to shield the user from having to deal with
D-Bus. But sometimes more information is really useful, for example when we're dealing
with unicast signals.

Unlike broadcast signals, unicast signals are sent from a specific D-Bus entity (identified
as `{dbus_name, object_path, interface_name, signal_name}`) to a receiver bus. Something on
the receiver dbus must register to receive the signal specifically from the sender bus,
interface and object.

Unicast signals are always associated with an `easydbuspp::object`, so they will always be
sent from the object's path and interface. But to which destination bus?

Let's add a unicast signal with an `std::string` parameter to our object:

```cpp
auto unicast_signal = object.add_unicast_signal<std::string>("UnicastSignal");
```

Let's assume we have set up a proxy that has subscribed to a unicast signal from our
object:

```cpp
easydbuspp::org_freedesktop_dbus_proxy dbus_proxy(proxy_session_manager);
std::string                            unique_bus_name =
    dbus_proxy.unique_bus_name("net.test.EasyDBuspp.Test");

proxy_session_manager.signal_subscribe(
    "UnicastSignal",
    [&proxy_session_manager](const std::string& s) {
        std::cout << "Got signal UnicastSignal: ['" << s << "']" << std::endl;
        proxy_session_manager.stop();
    },
    unique_bus_name, "net.test.EasyDBuspp.TestInterface", "/net/test/EasyDBuspp/TestObject");
```

Now, the way to call a signal from a method would be:

```cpp
unicast_signal(destination_bus_name, "Unicast signal emitted!");
```

But what do we put in `destination_bus_name`? We could trigger the signal from a method
that takes an `std::string` parameter, send in the destination bus name from the caller,
and use that value for the signal. But that opens us up to cheating: maybe we only want
to send out signals to certain busses, and the real caller bus is not the bus sent as a
parameter.

In that case, we want the actual bus that D-Bus is seeing as the caller bus when handling
our method. There's no way around it, we need that passed into our method directly by
D-Bus.

But that means that this new parameter cannot participate in the D-Bus introspection
information. It also means that it cannot be a required parameter, since most users of the
library would find it both unnecessary and cumbersome to deal with D-Bus data they don't
care about.

The imperfect solution is the `easydbuspp::dbus_context` type, which is a struct:

```cpp
struct dbus_context {
    std::string   bus_name;
    std::string   interface_name;
    object_path_t object_path;
    std::string   name;
};
```

All you need to do is have one parameter in your callable with this type. The library
will automatically detect it and fill it in. It won't show up as a method parameter in
introspection, and it will be treated as non-existent as far as the input parameter
names are concerned.

```cpp
object.add_method("EmitUnicastSignal", [&unicast_signal](const easydbuspp::dbus_context& dc) {
    unicast_signal(dc.bus_name, "Unicast signal emitted!");
});
```

This is probably a niche thing that most people won't have to worry about knowing or using.

*If at all possible, prefer the cleaner `pre_request_handler()` mechanism.*

## D-Bus $\leftrightarrow$ C++ type mapping

| D-Bus         | C++                              |
| ------------- | -------------------------------- |
| `b`           | `bool`                           |
| `n`           | `int16_t`                        |
| `q`           | `uint16_t`                       |
| `i`           | `int32_t`                        |
| `u`           | `uint32_t`                       |
| `x`           | `int64_t`                        |
| `t`           | `uint64_t`                       |
| `d`           | `double`, `float`                |
| `y`           | `std::byte`                      |
| `s`           | `std::string`                    |
| `o`           | `object_path_t`                  |
| `v`           | `std::variant`                   |
| `a`           | `std::vector`                    |
| `()`          | `std::tuple`, `std::pair`        |
| `a{}`         | `std::map`, `std::unordered_map` |

You may have noticed that `object_path_t` does not look like a standard C++ type.
But it is just an alias for `std::filesystem::path`, so in reality it is.

## Building

```
meson setup build
cd build
meson compile
```

Running the automated unit tests (optional), from the `build` directory:

```
meson test
```

## Built with

* [glib-2](https://docs.gtk.org/glib/) - D-Bus implementation

## Thanks

Special thanks to David Sommerseth, author of [gdbuspp](https://codeberg.org/OpenVPN/gdbuspp) for inspiration and
encouragement!

## Authors

* **Răzvan Cojocaru** [rzvncj](https://github.com/rzvncj)
