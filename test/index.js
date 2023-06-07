const Brightness = require("../index.js");
const {describe, it} = require("mocha");
const {expect} = require("chai");
const {sample, random} = require("lodash");
const _ = require("lodash");

describe("lumi", function () {
    this.timeout(0);
    it("should return available monitors", () => {
        const monitors = Brightness.monitors();
        expect(monitors).to.not.be.empty;
    });
    it("should return brightness", async function () {
        const monitors = Brightness.monitors();
        const monitor = sample(monitors);
        const {success, brightness} = await Brightness.get(monitor.id);
        if (success) return expect(brightness).to.be.a("number");
        return expect(brightness).to.be.null;
    });
    it("should set brightness for monitor", async function () {
        const monitors = Brightness.monitors();
        const monitor = sample(monitors);
        const {success, brightness} = await Brightness.get(monitor.id);

        if (brightness === null) return expect(success).to.be.false;

        const {success: ableToSetBrightness} = await Brightness.set(monitor.id, random(100));

        if (brightness !== 0) await Brightness.set(monitor.id, brightness);

        return expect(ableToSetBrightness).to.be.true;
    });
    it("should set brightness with valid config", async () => {
        const monitors = Brightness.monitors();
        const config = _.reduce(monitors, (config, {id}) => ({
            ...config,
            [id]: random(100)
        }), {});
        const {success} = await Brightness.set(config);
        expect(success).to.be.true;
    });
    it("should fail to set brightness for non-existent monitor", async () => {
        const {success} = await Brightness.set("fake-monitor-id", 50);
        expect(success).to.be.false;
    });
});