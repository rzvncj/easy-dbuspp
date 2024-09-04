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
easydbuspp::session_manager session_manager {easydbuspp::bus_type_t::SESSION, "org.gtk.GDBus.Test"};
easydbuspp::object object {session_manager, "org.gtk.GDBus.TestInterface", "/org/gtk/GDBus/TestObject"};
```

The second parameter to `session_manager`'s constructor is the bus name. Then we create an object which
uses the `session_manager` instance, with a given interface name and a D-Bus object path.

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

And that's it! You can introspect and use your shiny new D-Bus object with one of a variety of
command line or GUI tools:

* [d-spy](https://apps.gnome.org/Dspy/)
* [qtdbusviewer](https://doc.qt.io/qt-6/qdbusviewer.html)
* [gdbus](https://manpages.ubuntu.com/manpages/focal/man1/gdbus.1.html)

### Proxies: the other side of the coin

Now that we have a service up and running, we can connect to it using `easydbuspp::proxy`.

Here's how to set one up:

```cpp
easydbuspp::session_manager session_manager {easydbuspp::bus_type_t::SESSION};
easydbuspp::proxy           proxy(session_manager, "org.gtk.GDBus.Test", "org.gtk.GDBus.TestInterface",
                                  "/org/gtk/GDBus/TestObject");
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
proxy.cached_property("ReadWriteStringProp", "Value to set on remote object");
```

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
auto a = std::async(std::launch::async, [&session_manager] {
    session_manager.run();
});
```

leaving the main thread free to trigger the signal:

```cpp
proxy.call<void>("TriggerBroadcastSignal");
```

Once the signal is triggered, the callback is called, which stops the main thread loop,
which allows the process to exit.

## Building

```
meson setup build
cd build
meson compile
```

## Built with

* [glib-2](https://docs.gtk.org/glib/) - D-Bus implementation

## Authors

* **Răzvan Cojocaru** [rzvncj](https://github.com/rzvncj)
