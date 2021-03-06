
.. _comm-framework:

======================================
Broker-Enabled Communication Framework
======================================

.. rst-class:: opening

    Bro can now use the `Broker Library
    <../components/broker/README.html>`_ to exchange information with
    other Bro processes.  To enable it run Bro's ``configure`` script
    with the ``--enable-broker`` option.  Note that a C++11 compatible
    compiler is required as well as the `C++ Actor Framework
    <http://actor-framework.org/>`_.

.. contents::

Connecting to Peers
===================

Communication via Broker must first be turned on via
:bro:see:`Comm::enable`.

Bro can accept incoming connections by calling :bro:see:`Comm::listen`
and then monitor connection status updates via
:bro:see:`Comm::incoming_connection_established` and
:bro:see:`Comm::incoming_connection_broken`.

.. btest-include:: ${DOC_ROOT}/frameworks/comm/connecting-listener.bro

Bro can initiate outgoing connections by calling :bro:see:`Comm::connect`
and then monitor connection status updates via
:bro:see:`Comm::outgoing_connection_established`,
:bro:see:`Comm::outgoing_connection_broken`, and
:bro:see:`Comm::outgoing_connection_incompatible`.

.. btest-include:: ${DOC_ROOT}/frameworks/comm/connecting-connector.bro

Remote Printing
===============

To receive remote print messages, first use
:bro:see:`Comm::subscribe_to_prints` to advertise to peers a topic
prefix of interest and then create an event handler for
:bro:see:`Comm::print_handler` to handle any print messages that are
received.

.. btest-include:: ${DOC_ROOT}/frameworks/comm/printing-listener.bro

To send remote print messages, just call :bro:see:`Comm::print`.

.. btest-include:: ${DOC_ROOT}/frameworks/comm/printing-connector.bro

Notice that the subscriber only used the prefix "bro/print/", but is
able to receive messages with full topics of "bro/print/hi",
"bro/print/stuff", and "bro/print/bye".  The model here is that the
publisher of a message checks for all subscribers who advertised
interest in a prefix of that message's topic and sends it to them.

Message Format
--------------

For other applications that want to exchange print messages with Bro,
the Broker message format is simply:

.. code:: c++

    broker::message{std::string{}};

Remote Events
=============

Receiving remote events is similar to remote prints.  Just use
:bro:see:`Comm::subscribe_to_events` and possibly define any new events
along with handlers that peers may want to send.

.. btest-include:: ${DOC_ROOT}/frameworks/comm/events-listener.bro

To send events, there are two choices.  The first is to use call
:bro:see:`Comm::event` directly.  The second option is to use
:bro:see:`Comm::auto_event` to make it so a particular event is
automatically sent to peers whenever it is called locally via the normal
event invocation syntax.

.. btest-include:: ${DOC_ROOT}/frameworks/comm/events-connector.bro

Again, the subscription model is prefix-based.

Message Format
--------------

For other applications that want to exchange event messages with Bro,
the Broker message format is:

.. code:: c++

    broker::message{std::string{}, ...};

The first parameter is the name of the event and the remaining ``...``
are its arguments, which are any of the support Broker data types as
they correspond to the Bro types for the event named in the first
parameter of the message.

Remote Logging
==============

.. btest-include:: ${DOC_ROOT}/frameworks/comm/testlog.bro

Use :bro:see:`Comm::subscribe_to_logs` to advertise interest in logs
written by peers.  The topic names that Bro uses are implicitly of the
form "bro/log/<stream-name>".

.. btest-include:: ${DOC_ROOT}/frameworks/comm/logs-listener.bro

To send remote logs either use :bro:see:`Log::enable_remote_logging` or
:bro:see:`Comm::enable_remote_logs`.  The former allows any log stream
to be sent to peers while the later toggles remote logging for
particular streams.

.. btest-include:: ${DOC_ROOT}/frameworks/comm/logs-connector.bro

Message Format
--------------

For other applications that want to exchange logs messages with Bro,
the Broker message format is:

.. code:: c++

    broker::message{broker::enum_value{}, broker::record{}};

The enum value corresponds to the stream's :bro:see:`Log::ID` value, and
the record corresponds to a single entry of that log's columns record,
in this case a ``Test::INFO`` value.

Tuning Access Control
=====================

By default, endpoints do not restrict the message topics that it sends
to peers and do not restrict what message topics and data store
identifiers get advertised to peers.  These are the default
:bro:see:`Comm::EndpointFlags` supplied to :bro:see:`Comm::enable`.

If not using the ``auto_publish`` flag, one can use the
:bro:see:`Comm::publish_topic` and :bro:see:`Comm::unpublish_topic`
functions to manipulate the set of message topics (must match exactly)
that are allowed to be sent to peer endpoints.  These settings take
precedence over the per-message ``peers`` flag supplied to functions
that take a :bro:see:`Comm::SendFlags` such as :bro:see:`Comm::print`,
:bro:see:`Comm::event`, :bro:see:`Comm::auto_event` or
:bro:see:`Comm::enable_remote_logs`.

If not using the ``auto_advertise`` flag, one can use the
:bro:see:`Comm::advertise_topic` and :bro:see:`Comm::unadvertise_topic`
to manupulate the set of topic prefixes that are allowed to be
advertised to peers.  If an endpoint does not advertise a topic prefix,
the only way a peers can send messages to it is via the ``unsolicited``
flag of :bro:see:`Comm::SendFlags`  and choosing a topic with a matching
prefix (i.e. full topic may be longer than receivers prefix, just the
prefix needs to match).

Distributed Data Stores
=======================

There are three flavors of key-value data store interfaces: master,
clone, and frontend.

A frontend is the common interface to query and modify data stores.
That is, a clone is a specific type of frontend and a master is also a
specific type of frontend, but a standalone frontend can also exist to
e.g. query and modify the contents of a remote master store without
actually "owning" any of the contents itself.

A master data store can be be cloned from remote peers which may then
perform lightweight, local queries against the clone, which
automatically stays synchronized with the master store.  Clones cannot
modify their content directly, instead they send modifications to the
centralized master store which applies them and then broadcasts them to
all clones.

Master and clone stores get to choose what type of storage backend to
use.  E.g. In-memory versus SQLite for persistence.  Note that if clones
are used, data store sizes should still be able to fit within memory
regardless of the storage backend as a single snapshot of the master
store is sent in a single chunk to initialize the clone.

Data stores also support expiration on a per-key basis either using an
absolute point in time or a relative amount of time since the entry's
last modification time.

.. btest-include:: ${DOC_ROOT}/frameworks/comm/stores-listener.bro

.. btest-include:: ${DOC_ROOT}/frameworks/comm/stores-connector.bro

In the above example, if a local copy of the store contents isn't
needed, just replace the :bro:see:`Store::create_clone` call with
:bro:see:`Store::create_frontend`.  Queries will then be made against
the remote master store instead of the local clone.

Note that all queries are made within Bro's asynchrounous ``when``
statements and must specify a timeout block.
