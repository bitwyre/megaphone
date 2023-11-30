#pragma once

class FBHandler {
public:
	FBHandler() = default;
	~FBHandler() = default;

	FBHandler(const FBHandler&) = delete;
	FBHandler(const FBHandler&) = delete;
	FBHandler(FBHandler&&) noexcept = delete;
	FBHandler(FBHandler&&) noexcept = delete;

	[[nodiscard]];

private:
};
