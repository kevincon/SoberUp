/*global window*/

Pebble.addEventListener("showConfiguration", function(e) {
    // If the weight and gender are note defined, make them their default values
    var store = window.localStorage;
    if (store.getItem("weight") === undefined) {
        store.setItem("weight", 150);
    }
    if (store.getItem("gender") === undefined) {
        store.setItem("gender", "male");
    }
    if (store.getItem("signedEULA") === undefined) {
        store.setItem("signedEULA", false);
    }
    var weight = store.getItem("weight");
    var gender = store.getItem("gender");
    var signed = store.getItem("signedEULA");

    var url = "http://reptar-on-ice.herokuapp.com/" +
        "?weight=" + weight +
        "&gender=" + gender +
        "&signedEULA=" + signed;

    console.log(url);
    Pebble.openURL(url);
});

Pebble.addEventListener("webviewclosed",
    function(e) {
        var configuration = JSON.parse(decodeURIComponent(e.response));

        if (configuration.gender && configuration.weight) {
            // Save the values from the webframe to local storage
            var store = window.localStorage;
            store.setItem("weight", configuration.weight);
            store.setItem("gender", configuration.gender);
            store.setItem("signedEULA", configuration.signedEULA);

            // Update the config to make parsing easier in C
            if (configuration.gender == "male") {
                configuration.gender = 1;
            } else {
                configuration.gender = 0;
            }
            // Send the config to the pebble
            console.log("Gender: " + configuration.gender);
            console.log("Weight: " + configuration.weight);
            console.log("Accepted Eula: " + configuration.signedEULA);

            var transactionId = Pebble.sendAppMessage(
                configuration,
                function(e) {
                    console.log("Successfully delivered message with transactionId=" + e.data.transactionId);
                },
                function(e) {
                    console.log("Unable to deliver message with transactionId=" + e.data.transactionId + " Error is: " + e.error.message);
                }
            );
        }
    }
);