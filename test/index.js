const lumi = require("../index.js");
const {describe, it} = require("mocha");
const {expect} = require("chai");
const {sample, random} = require("lodash");
const _ = require("lodash");

describe("lumi", function () {
    this.timeout(0);
    it("should return available monitors", () => {
        const monitors = lumi.monitors();
        expect(monitors).to.not.be.empty;
    });
    it("should return brightness", async function () {
        const monitors = lumi.monitors();
        const monitor = sample(monitors);
        const {success, brightness} = await lumi.get(monitor.id);
        if (success) return expect(brightness).to.be.a("number");
        return expect(brightness).to.be.null;
    });
    it("should set global brightness", async function () {
        const {success} = await lumi.set(lumi.GLOBAL, 50);
        return expect(success).to.be.true;
    });
    it("should set brightness", async function () {
        const monitors = lumi.monitors();
        const monitor = sample(monitors);
        const {success, brightness} = await lumi.get(monitor.id);

        if (brightness === null) return expect(success).to.be.false;

        const {success: ableToSetlumi} = await lumi.set(monitor.id, random(100));

        if (brightness !== 0) await lumi.set(monitor.id, brightness);

        return expect(ableToSetlumi).to.be.true;
    });
    it("should set brightness with valid config", async () => {
        const monitors = lumi.monitors();
        const config = _.reduce(monitors, (config, {id}) => ({
            ...config,
            [id]: random(100)
        }), {});
        const {success} = await lumi.set(config);
        expect(success).to.be.true;
    });
    it("should fail to set brightness for non-existent monitor", async () => {
        const {success} = await lumi.set("fake-monitor-id", 50);
        expect(success).to.be.false;
    });
});